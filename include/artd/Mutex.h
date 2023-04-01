#ifndef __artd_Mutex_h
#define __artd_Mutex_h 

#include "artd/thread_base.h"
#include "artd/artd_assert.h"
#include "artd/thread/OsThread.h"

#if defined(ARTD_THREAD_STD)
	#include <mutex>
#endif

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

class Mutex
{
protected:
    ARTD_API_JLIB_THREAD void forceLinkage();
public:

#pragma warning( push )
#pragma warning( disable : 26495)
	INL Mutex(const Mutex& m) { } // disallowed
	INL Mutex& operator=(const Mutex& m) { Mutex* mx = 0; return(*mx); } // disallowed
#pragma warning( pop )

	Mutex();
	~Mutex();
	void acquire();
	void release();
	bool tryAcquire();

	INL void synchro_release() {  // for the "synchronized" item
		release();
	}

private:
	typedef std::recursive_mutex OSMutex;
	OSMutex impl_;

#ifdef ARTD_DEBUG
	int lockCount_ = 0;
#endif

};

#undef INL

#include "artd/thread/Mutex.inl"


ARTD_END

#endif // __artd_Mutex_h
