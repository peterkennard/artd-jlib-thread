#ifndef __artd_Semaphore_h
#define __artd_Semaphore_h

#include "artd/thread_base.h"
#include "artd/OsThread.h"
#include "artd/Mutex.h"

#if defined(ARTD_THREAD_STD)
    #include <mutex>
	#include <condition_variable> 
#endif

ARTD_BEGIN

#ifdef ARTD_WINDOWS
	#define ARTD_SEMAPHORE_WAIT_PRECISION (1.0 / 1000)
#else
	#define ARTD_SEMAPHORE_WAIT_PRECISION (1.0 / 1000000)
#endif

#define INL ARTD_ALWAYS_INLINE

class ARTD_API_JLIB_THREAD Semaphore
{
public:
#pragma warning( push )
#pragma warning( disable : 26495)

	explicit Semaphore(int maxCount = 1, int initCount = 0);
	~Semaphore();
	void signal();
	/** @brief returns 0 if signaled, non 0 if other error (eWAIT_INTERRUPTED) */
	int wait();
	/** @brief returns 0 if signaled, non 0 if timed out or other error */
	int timed_wait(double msec);

private:

	INL Semaphore(const Semaphore &m) { } // disallowed
#pragma warning( pop ) 
	INL Semaphore &operator=(const Semaphore &m) { return(*this); } // disallowed
	
	class Impl;
	Impl& I();

#if defined(ARTD_THREAD_STD)
	
	typedef std::mutex OSMutex;

	union
	{
		char data[sizeof(OSMutex) + sizeof(std::condition_variable) + sizeof uint32_t];
		int64_t align;
	} impl_;

#elif defined(ARTD_THREAD_WINDOWS)
	void *m_sema;
	int m_waitCount;
	Mutex m_mutex;
	bool m_semaDeleted;
	union
	{
		char data[4];
		int32_t align;
	} impl_;

#elif defined(ARTD_THREAD_PTHREAD)
	Mutex m_mutex;
	int m_maxCount;
	int m_waitCount;
	bool m_semaDeleted;
	union
	{
		#ifdef ARTD_64BIT
			char data[32]; // sizeof(sem_t)
		#else
			char data[16]; // sizeof(sem_t)
		#endif
		int64_t align;
	} impl_;
	class Impl;
	ARTD_ALWAYS_INLINE Impl& I() const { return(*(Impl*)&impl_); }
#endif

};

#undef INL

ARTD_END

#endif // __artd_Semaphore_h
