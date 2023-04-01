#ifndef __artd_WaitableSignal_h
#define __artd_WaitableSignal_h

#include "artd/Semaphore.h"

ARTD_BEGIN


#ifndef ARTD_SINGLE_THREADED

class ARTD_API_JLIB_THREAD WaitableSignal
	: protected Semaphore
{
	bool signalled_;
	public:

	WaitableSignal()
			: Semaphore(1,0)
	{
		signalled_ = false;
	}
	~WaitableSignal() {
		signal();
	}

	/**
	 * set the signal to true so the waiter
	 * will be woken up.
	 */
	ARTD_ALWAYS_INLINE void signal() {
		signalled_ = true;
		Semaphore::signal();
	}

	/**
	 * wait forever for signal
	 * @return 0 if signalled, or eWAIT_INTERUPTED
	 */
	ARTD_ALWAYS_INLINE int waitOnSignal() {
		while (!signalled_) {
			int ret = Semaphore::wait();
			if(ret != 0) {
				return(ret);
			}
		}
		signalled_ = false;
		return(0);
	}

	/**
	 * wait on signal with timeout
	 * @param timeoutMillis max time to wait
	 * @return 0 if signaled, or eWAIT_TIMEOUT || eWAIT_INTERRUPTED
	 */
	ARTD_ALWAYS_INLINE int waitOnSignal(int timeoutMillis) {
		while (!signalled_) {
			int ret = Semaphore::timed_wait(timeoutMillis);
			if (ret != 0) {
				return(ret);
			}
		}
		signalled_ = false;
		return(0);
	}
};

#else // single threaded ie: do nothing.
class ARTD_API_JLIB_THREAD WaitableSignal
	: protected Semaphore
{
public:

	WaitableSignal() {
	}
	~WaitableSignal() {
	}

	/**
     * set the signal to true so the waiter
     * will be woken up.
     */
	ARTD_ALWAYS_INLINE void signal() {
	}

	/**
     * wait forever for signal
     * @return 0 if signalled, or eWAIT_INTERUPTED
     */
	ARTD_ALWAYS_INLINE int waitOnSignal() {
		return(0);
	}

	/**
     * wait on signal with timeout
     * @param timeoutMillis max time to wait
     * @return 0 if signaled, or eWAIT_TIMEOUT || eWAIT_INTERRUPTED
     */
	ARTD_ALWAYS_INLINE int waitOnSignal(int timeoutMillis) {
		return(0);
	}
};

#endif


ARTD_END

#endif // __artd_WaitableSignal_h
