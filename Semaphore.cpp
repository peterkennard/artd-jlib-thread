#include "artd/Semaphore.h"
#include "artd/Logger.h"
#include "artd/thread/StallMonitor.h"
#include <math.h>


#if defined (ARTD_THREAD_STD)

#include <mutex>
#include <condition_variable>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE // note methods in this Impl class are only called by Semaphore

class Semaphore::Impl {
public:
	std::mutex mutex_;
	std::condition_variable cv_;
	uint32_t count_; // Initialized as locked 0.

	INL Impl(int32_t initCount) : count_(initCount) {
	}

	INL void signal() {
		std::unique_lock<decltype(mutex_)> lock(mutex_);
		++count_;
		cv_.notify_one();
	}

	INL int timed_wait(double msec) {

		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		if (msec < 0) {
			end += std::chrono::milliseconds((uint64_t)(~(0x7FFFFFFF)));
		}
		else {
			end += std::chrono::microseconds((uint64_t)(msec * 1000));
		}

		std::unique_lock<decltype(mutex_)> lock(mutex_);
		OsThread* ot = OsThread::currentOsThread();
		if (ot) {
			if (ot->isInterrupted()) {
				ot->_set_interruptable_waiter_(0);
				return(OsThread::eWAIT_INTERRUPTED);
			}
			while (!count_) {  // Handle spurious wake-ups.
				ot->_set_interruptable_waiter_(&cv_);
				std::cv_status  ret = cv_.wait_until(lock, end);
				ot->_set_interruptable_waiter_(0);
				if (ot->isInterrupted()) {
					return(OsThread::eWAIT_INTERRUPTED);
				}
				if (ret == std::cv_status::timeout) {
					return(OsThread::eWAIT_TIMEOUT);
				}
			}
		}
		else {
			while (!count_) {  // Main thread - no handle assigned !!! - can't inturrupt it Handle spurious wake-ups.
				AD_LOG(info) << "spurrious wake up ???";

				std::cv_status  ret = cv_.wait_until(lock, end);
				if (ret == std::cv_status::timeout) {
					return(OsThread::eWAIT_TIMEOUT);
				}
			}
		}
		--count_;
		return(0);
	}
	INL int wait() {
		return(timed_wait(-1));
	}

	bool try_wait() {
		std::unique_lock<decltype(mutex_)> lock(mutex_);
		if (count_) {
			--count_;
			return true;
		}
		return false;
	}
};

INL Semaphore::Impl& Semaphore::I() {
	return(*reinterpret_cast<Impl*>(this));
}

Semaphore::Semaphore(int maxCount, int initCount) {

	extern char size_test[sizeof(Impl) <= sizeof(impl_) ? 1 : -1];
	initCount = (initCount > maxCount) ? maxCount : initCount;
	new(&I()) Impl(initCount);
}

Semaphore::~Semaphore() {
	I().~Impl();
}

void
Semaphore::signal() {
	I().signal();
}

int Semaphore::wait()
{
	StallMonitor stallChecker;
	return(I().wait());
}

int
Semaphore::timed_wait(double msec)
{
	StallMonitor stallChecker;
	return(I().timed_wait(msec));
}

ARTD_END

// TODO: do we need this any more ?
#elif defined(ARTD_THREAD_PTHREAD)

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#ifdef __GNUC__
    #pragma GCC diagnostic ignored "-Wunused-variable"
#endif

ARTD_BEGIN

Semaphore::Semaphore(int maxCount, int initCount)
{
	extern char size_test[sizeof(sem_t) <= sizeof(impl_) ? 1 : -1];
	char *privateDataPtr = impl_.data;
	sem_t *s = (sem_t *) privateDataPtr;
	m_maxCount = maxCount;
	initCount = (initCount > m_maxCount) ? m_maxCount : initCount;
	int res = sem_init(s, 0, initCount);
	ARTD_ASSERT(!res);
	m_waitCount = 0;
	m_semaDeleted = false;
}

Semaphore::~Semaphore()
{
	char *privateDataPtr = impl_.data;
	sem_t *s = (sem_t *) privateDataPtr;
	m_semaDeleted = true;
	while (m_waitCount > 0)
	{
		sem_post(s);
		usleep(1000);
	}
	m_mutex.acquire();
	int res = sem_destroy(s);
	ARTD_ASSERT(!res);
	m_mutex.release();
}

void Semaphore::signal()
{
	StallMonitor stallChecker;
	m_mutex.acquire();
	char *privateDataPtr = impl_.data;
	sem_t *s = (sem_t *) privateDataPtr;
	int val = 0;
	int res = sem_getvalue(s, &val);
	ARTD_ASSERT(!res);
	if (val < m_maxCount)
	{
		res = sem_post(s);
		ARTD_ASSERT(!res);
	}
	m_mutex.release();
}

void Semaphore::wait()
{
	StallMonitor stallChecker;
	if (m_semaDeleted)
	{
		ARTD_ASSERT(0); // attempting to wait on a deleted semaphore
		return;
	}
	m_mutex.acquire();
	m_waitCount++;
	m_mutex.release();
	char *privateDataPtr = impl_.data;
	sem_t *s = (sem_t *) privateDataPtr;
	int res;
	do
	{
		res = sem_wait(s);
		// older versions of gdb seem to cause an EINTR error return
	} while (res == -1 && errno == EINTR); 
	ARTD_ASSERT(!res);
	m_mutex.acquire();
	m_waitCount--;
	m_mutex.release();
	// NOTE: this will be refactored into a return result
	ARTD_ASSERT(!m_semaDeleted); // semaphore was deleted during wait
}

bool Semaphore::timed_wait(double msec)
{
	StallMonitor stallChecker;
	if (m_semaDeleted)
	{
		ARTD_ASSERT(0); // attempting to wait on a deleted semaphore
		return false;
	}
	m_mutex.acquire();
	m_waitCount++;
	m_mutex.release();
	char *privateDataPtr = impl_.data;
	sem_t *s = (sem_t *) privateDataPtr;
	unsigned long long nsec = (unsigned long long) ceil(msec * 1000000.0);
	struct timespec ts;
	int res = clock_gettime(CLOCK_REALTIME, &ts);
	ARTD_ASSERT(!res);
	unsigned long long sec = nsec / 1000000000;
	nsec -= sec * 1000000000;
	ts.tv_sec += (time_t) sec;
	ts.tv_nsec += (long) nsec;
	if (ts.tv_nsec > 1000000000)
	{
		ts.tv_nsec -= 1000000000;
		ts.tv_sec++;
	}
	res = sem_timedwait(s, &ts);
	m_mutex.acquire();
	m_waitCount--;
	m_mutex.release();
	// NOTE: this will be refactored into a return result
	ARTD_ASSERT(!m_semaDeleted); // semaphore was deleted during wait
	return (res == 0);
}

ARTD_END

// TODO: any advantage to this ?
#elif defined(ARTD_THREAD_WINDOWS)

#include <artd/platform_specific.h>

ARTD_BEGIN

Semaphore::Semaphore(int maxCount, int initCount)
{
	initCount = (initCount > maxCount) ? maxCount : initCount;
	m_sema = CreateSemaphore(0, initCount, maxCount, 0);
	ARTD_ASSERT(m_sema); // semaphore creation failed
	m_waitCount = 0;
	m_semaDeleted = false;
}

Semaphore::~Semaphore()
{
	m_semaDeleted = true;
// TODO: This check is invalid if running in a DLL, and the semaphore is being
// deleted from within a static destructor.  The VC CRT terminates any threads
// created by the DLL before static destructors are called.  We need to store a
// list of threadid that can be safely checked, to see if any threads are
// -unterminated- and waiting on the semaphore.  As a bonus, find a workaround
// for that horrible horrible behaviour from the CRT.
#ifndef _DLL
	int delayMsec = 1;
	bool wereWaitingThreads = (m_waitCount > 0);
	while (m_waitCount > 0)
	{
		int waitCount = m_waitCount;
		ReleaseSemaphore(m_sema, 1, 0);
		Sleep(delayMsec);
		if (m_waitCount == waitCount)
		{
			if (delayMsec == 100)
				break;
			delayMsec = 100;
		}
	}
	ARTD_ASSERT(!wereWaitingThreads); // deleted a semaphore that threads were waiting on
#endif
	m_mutex.acquire();
	BOOL res = CloseHandle(m_sema);
	ARTD_ASSERT(res); // semaphore delete failed
	m_mutex.release();
}

void Semaphore::signal()
{
	StallMonitor stallChecker;
	::ReleaseSemaphore(m_sema, 1, 0);
}

int Semaphore::wait()
{
	return(timed_wait(-1));
}

int 
Semaphore::timed_wait(double msecd)
{
	unsigned int msec = (msecd < 0) ? INFINITE : (unsigned int)(msecd + .9999999);
	StallMonitor stallChecker;
	if (m_semaDeleted)
	{
		ARTD_ASSERT(0); // attempting to wait on a deleted semaphore
		return(-1); // general error
	}
	OsThread *ot = OsThread::currentOsThread();
	if (ot && ot->interrupted()) {
		return(OsThread::eWAIT_INTERRUPTED);
	}
	m_mutex.acquire();
	m_waitCount++;
	m_mutex.release();
	DWORD ret;
	if (ot) {
		HANDLE h[2];
		h[0] = ot->_get_interruptable_data_();
		h[1] = m_sema;
		ret = ::WaitForMultipleObjects(2, h, false, msec);
		if (ot->interrupted()) {
			ret = OsThread::eWAIT_INTERRUPTED;
		} else if (ret == 1) {
			ret = 0;
		} else {
			ret = OsThread::eWAIT_TIMEOUT;
		}
	}
	else {
		ret = ::WaitForSingleObject(m_sema, msec);
		ret = (ret == WAIT_OBJECT_0) ? 0 : OsThread::eWAIT_TIMEOUT;
	}
	m_mutex.acquire();
	m_waitCount--;
	m_mutex.release();

	ARTD_ASSERT(!m_semaDeleted); // semaphore was deleted during wait
	return(ret);
}

ARTD_END

#endif

