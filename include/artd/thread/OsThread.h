#ifndef __artd_thread_OsThread_h
#define __artd_thread_OsThread_h

#include "artd/thread_base.h"
#include "artd/int_types.h"
#include "artd/StringArg.h"
#include "artd/ObjectBase.h"
#include "artd/atomic_ops.h"

#if 0
#ifndef ARTD_THREADTYPE
	#define ARTD_THREADTYPE Unknown
#endif

// determine ARTD_THREADTYPE_Current from build set value -DARTD_THREADTYPE={Std,PThread,Windows,Boost}
#define ARTD_THREADTYPE_Unknown 0
#define ARTD_THREADTYPE_PThread 1
#define ARTD_THREADTYPE_Windows 2
#define ARTD_THREADTYPE_Boost 3
#define ARTD_THREADTYPE_Std 4
#define ARTD_THREADTYPE_Current ARTD_JOIN(ARTD_THREADTYPE_,ARTD_THREADTYPE)

// set default if not set by build
#if ARTD_THREADTYPE_Current == ARTD_THREADTYPE_Unknown
    #if defined (__linux__) || defined (__EMSCRIPTEN__)
	#define ARTD_THREAD_STD
// #define ARTD_THREAD_PTHREAD
    #elif defined(ARTD_WINDOWS)
		#define ARTD_THREAD_STD  // they have improved the std::mutex performance
		#define ARTD_THREAD_WINDOWS
	#endif
    // ARTD_THREAD_BOOST
#elif ARTD_THREADTYPE_Current == ARTD_THREADTYPE_Std
#define ARTD_THREAD_STD
#elif ARTD_THREADTYPE_Current == ARTD_THREADTYPE_Windows
	#define ARTD_THREAD_STD
	#define ARTD_THREAD_WINDOWS
#elif ARTD_THREADTYPE_Current == ARTD_THREADTYPE_Boost
    #define ARTD_THREAD_WINDOWS
#elif ARTD_THREADTYPE_Current == ARTD_THREADTYPE_PThread
    #define ARTD_THREAD_PTHREAD
#endif
#undef ARTD_THREADTYPE_Current

#endif


#define ARTD_THREAD_STD

ARTD_BEGIN

class Semaphore;
class OsThreadImpl;


class ARTD_API_JLIB_THREAD OsThread
{
private: 
	OsThread( const OsThread& b );    //noncopyable
	OsThread& operator = ( const OsThread& b );   //noncopyable

	void _set_interruptable_waiter_(void *waiter);
	void *_get_interruptable_data_();


protected:
    friend class OsThreadImpl;
    friend class Semaphore;

	OsThread(StringArg name = 0);
	virtual ~OsThread();

	// override for cleanup actions
	virtual void  onStop() {}

public:
	typedef void (*ThreadEntry)(void*);
	
	// error codes for sleep and wait calls
	static const int eWAIT_INTERRUPTED = -(0x0103);  // sleep, join 
	static const int eWAIT_TIMEOUT     = -(0x0104); // join

	// priority values for SetCurrentThreadPriority
	static const int    THREAD_MIN_PRIORITY = 1;

	enum Priority
	{
		Priority_Idle = THREAD_MIN_PRIORITY,
		Priority_Low,
		Priority_Normal,
		Priority_High,
		Priority_System
	};

	static const int THREAD_NORM_PRIORITY = Priority_Normal;
	static const int THREAD_MAX_PRIORITY = Priority_System;

	static bool       isMainThread();
	static int32_t    currentThreadId();

	static OsThread*  currentOsThread();

	static void	      yield();
    static int        sleep(int32_t millis=-1);

	
	__forceinline OsThreadImpl &I() 
		{ return(*(OsThreadImpl *)((void *)(&impl_))); }
	__forceinline const OsThreadImpl &I() const
		{ return(*(OsThreadImpl *)((void *)(&impl_))); }
	

	// thread entry point to execute
	// an argument can be passed via ptr 
	// name is an optional (can be 0), descriptive name for the thread, which is strncpy'd into the thread
	// structure in debug builds
	int               	start(OsThread::ThreadEntry threadEntry, void *arg);
    int               	stop();

	/** @brief set the name of this thread visible in the debugger */
	void            	setName(StringArg name);

	/** @brief get the name of this thread previously set with setName */
	const char *     	getName() const;
	
	/** @brief disconnect this object from the underlying 
	 *  operating system thread and leave the thread running 
	 *  the user is responsible for making sure the entry point
	 *  and associated data are not destroyed, and that it is properly
	 *  cleaned up when the thread exits.
	 */
	void            	detach();

	/** @brief join with this thread.
	 *  wait millis for the thread to exit, -1 == forever
	 * return 0 if success, eWAIT_TIMEOUT or eWAIT_INTERRUPTED
	 */
	int                 join( int32_t millis = -1 );

	int				    suspend();
	int				    resume();
    void			    interrupt();
    bool                isAlive();

	// test state of interupted flag without altering it.
    inline bool interrupted() {
        return((ARTD_ATOMIC_FETCH_AND_AND(&flags_, ~(thINTERRUPTED)) & thINTERRUPTED) != 0);    
    }
    inline bool isInterrupted() {
        return((ARTD_ATOMIC_FETCH_AND_OR(&flags_,0) & thINTERRUPTED) != 0);    
    }

protected:

	// state flags
	enum {
		thSTARTED     = 0x001,
		thINITIALIZING = 0x002,
		thALIVE       = 0x004,
		thINTERRUPTED = 0x008,
		thSUSPENDED   = 0x010,
		thCANCELING   = 0x020,
		thSTOPPED     = 0x040,
		thCLEANEDUP   = 0x080,
	};

    inline void setFlags(int32_t toSet) {
        ARTD_ATOMIC_FETCH_AND_OR(&flags_,toSet);   
    }
    inline int32_t clearFlags(int32_t toClear) {
        return(ARTD_ATOMIC_FETCH_AND_AND(&flags_,~(toClear)));   
    }
    inline int32_t getFlags(int32_t toGet) {
        return(ARTD_ATOMIC_FETCH_AND_OR(&flags_,0) & toGet);  
    }

    int32_t     flags_;
public:
    class ImplBuf {
    protected:
        void *x[6];
    };
    ImplBuf     impl_;
};


ARTD_END

#endif // __artd_thread_OsThread_h


