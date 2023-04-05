#ifndef __artd_CriticalSection_h
#define __artd_CriticalSection_h

#include "artd/thread_base.h"
#include <mutex>

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

/**
 * The CriticalSection class provides a simple non-recursive lock/unlock interface
 * around a platform-specific mutex implementation.  In this case ONLY the std::mutex
 * as the std::lib has gotten better over the years
 */
class CriticalSection
#ifndef ARTD_SINGLE_THREADED
	: public std::mutex
#endif
{
	typedef std::mutex super;
public:
	/** @brief Initialize new mutex. */
	INL CriticalSection() {}
	/** @brief Destroy mutex. */
	INL ~CriticalSection() {}

#ifdef ARTD_SINGLE_THREADED
	/** @brief Acquire a lock, waits until lock is available. */
	INL void acquire() {}
	/** @brief  Acquire a lock, only if available at the time of invocation. */
	INL bool tryAcquire() { return(true); }
	/** @brief Releases the lock once. */
	INL void release() {}
#else
	/** @brief Acquire a lock, waits until lock is available. */
	INL void acquire() {
		super::lock();
	}
	/** @brief  Acquire a lock, only if available at the time of invocation. */
	INL bool tryAcquire() {
		return(super::try_lock());
	}
	/** @brief Releases the lock once. */
	INL void release() {
		super::unlock();
	}
#endif
	INL void synchro_release() { 
		super::unlock();
	}

private:
    CriticalSection(const CriticalSection& rhs) {}; // can't copy
    void operator =(const CriticalSection& rhs) {}; // can't assign
};


#if 0  // TODO: Hmmm needed ?
class CriticalSectionPool {
public:
    CriticalSectionPool(int maxShares, int maxToPool);
    ~CriticalSectionPool();
    CriticalSection getSharedCS();
};
#endif

#undef INL

ARTD_END

#endif // __artd_CriticalSection_h
