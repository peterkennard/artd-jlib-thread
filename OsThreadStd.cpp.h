#include "artd/thread/OsThread.h"
#include "artd/artd_assert.h"
#include <thread>
#include <chrono>
#include "artd/Logger.h"
#include "artd/WaitableSignal.h"
#include "artd/pointer_math.h"
#include <mutex>
#include <condition_variable>
#include "artd/atomic_ops.h"
#include "artd/RcString.h"

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

static std::thread::native_handle_type invalidHandle_ = std::thread().native_handle();


class OsThreadImpl {
    friend class OsThread;
#ifdef __EMSCRIPTEN__
   #pragma GCC diagnostic ignored "-Wkeyword-macro"
#endif

public:
    
    std::thread thread_;
    artd::RcString name_;

    union {
        OsThread::ThreadEntry entry_;
        void *interruptableWaiter_;
    };    
    union {
        void       *arg_;
        Semaphore  *sleeper_;
    };

    INL OsThread *myBase() {
        return(ArtdToClass(OsThread, impl_, this));
    }
    INL int32_t getFlags(int32_t flags) {
        return(myBase()->getFlags(flags));
    }
    INL void setFlags(int32_t flags) {
        myBase()->setFlags(flags);
    }
    INL void clearFlags(int32_t flags) {
        myBase()->clearFlags(flags);
    }

	INL OsThreadImpl() : thread_() {
        
        entry_ = nullptr;   
        arg_ = nullptr;
        myBase()->flags_ = OsThread::thINITIALIZING;
    }
    void killThread() {
        if(*((int64_t*)&thread_) != 0) {
            thread_.detach();
            thread_.~thread();
        }
        memset(&thread_, 0, sizeof(thread_));
    }
	INL ~OsThreadImpl() {
        // AD_LOG(debug) << "deleting thread " <<  (void *)myBase();
        interrupt();
        if(getFlags(OsThread::thINITIALIZING)  == 0) {
            deleteZ(sleeper_);
        }
        killThread();
    }
	static void setCurrentThreadName(const char *name);
	
    INL bool started() {
	    return(getFlags(OsThread::thSTARTED) != 0);
    }

    INL bool isAlive() {
	    return(thread_.native_handle() != invalidHandle_ && getFlags(OsThread::thALIVE) != 0);
    }

    /**
      * TODO: this should be signalled instead of busy wait.
      */
    int join( int32_t timeout )
    {
        while(isAlive()) 
	    {
            if(!thread_.joinable()) {
                return(0);
            }
		    if(timeout > 5) {
			    if(OsThread::sleep(5) == OsThread::eWAIT_INTERRUPTED) {
			        return(OsThread::eWAIT_INTERRUPTED);
                }
			    timeout -= 5;
		    } else if(timeout <= 0) {
			    if(timeout < 0) {
				    continue;
			    }
			    return(OsThread::eWAIT_TIMEOUT);
		    } else {
		        if(OsThread::sleep(timeout) == OsThread::eWAIT_INTERRUPTED) {
		            return(OsThread::eWAIT_INTERRUPTED);
                }
			    timeout = 0;
		    }
	    }
        OsThread &oth = *myBase();
	    return(0);
    }

    INL void interrupt() {
        setFlags(OsThread::thINTERRUPTED);
        void *iw = ARTD_ATOMIC_GET_PTR(&interruptableWaiter_);
        if(iw) {
            ((std::condition_variable *)iw)->notify_one();
        }
    }
    INL void setWaiter(void *waiter) {
        ARTD_ATOMIC_SET_PTR(&interruptableWaiter_,waiter);
    }

private:

    INL OsThread::ThreadEntry getEntry() {
        OsThread::ThreadEntry e = (OsThread::ThreadEntry)entry_;
        entry_ = 0;
        return(e);
    }
    INL void *getEntryArg() {
        void *e = arg_;
        arg_ = 0;
		setFlags(OsThread::thALIVE);
		clearFlags(OsThread::thINITIALIZING | OsThread::thSUSPENDED);
        return(e);
    }

    INL void cleanup()
	{
		if(!(getFlags(OsThread::thCLEANEDUP)))
		{
            bool wasStopped = getFlags(OsThread::thSTOPPED) != 0;
			setFlags(OsThread::thCLEANEDUP | OsThread::thSTOPPED);
			clearFlags(~(OsThread::thCLEANEDUP | OsThread::thSTOPPED));
			
            killThread();
            if(!wasStopped) {
                myBase()->onStop();
            }
		}
	}

    INL void run()  {
        _currentOsThread_ = myBase();
        try {
            (*getEntry())(getEntryArg());
            // AD_LOG(info) << myBase()->getName() << " exiting";
        } catch(...) {
            AD_LOG(info) << "uncaught exception in thread " << (void *)myBase();
        }
        cleanup();
    }

    static void threadEntry(OsThreadImpl *me) {
        me->run();
    }

public:

    int start(OsThread::ThreadEntry entry, void *arg)  {
    	if ( entry == nullptr ) {
            return(-1);
        }
        if (started()) {
	    ARTD_ASSERT(0 && "OsThread: thread already started\n" );
    	    return(-1);
	    }
        // AD_LOG(debug) << "Starting thread " << ((void *)myBase()); 
        setFlags(OsThread::thSTARTED);
        entry_ = entry;
        arg_= arg;
        thread_.~thread();
        new(&thread_) std::thread(&threadEntry,this);
        return(0);
    }

    int
    sleep(int32_t milli)  {
            
        OsThread *ct = myBase();

        if(sleeper_ == nullptr)  {
            sleeper_ = new Semaphore(1,0);
        }
        int ret = sleeper_->timed_wait(milli);
        if(ret) {
            if(ret == OsThread::eWAIT_INTERRUPTED) {
                return(ret);
            }
        }
        return(0);
    }

    int stop() {    
        OsThread *ct = myBase();        
        ct->setFlags(OsThread::thCANCELING);
        return(0);
    }
    INL void detach() {
    }

    INL int suspend() {
        // TODO: not implemented ...
        return(0);
    }

    INL int resume() {
        // TODO: not implemented ...
        return(0);
    }


};

#undef INL


OsThread::OsThread(StringArg name)
{
	ARTD_STATIC_ASSERT((sizeof(OsThreadImpl) <= sizeof(impl_)));
    flags_ = 0;
	new(&I()) OsThreadImpl();
	setName(name);
}
OsThread::~OsThread() {
	I().~OsThreadImpl();
}
std::thread::id
OsThread::currentThreadId() {
//    ARTD_STATIC_ASSERT(sizeof(int32_t) == sizeof());
    return(std::this_thread::get_id());
}
void 
OsThread::setName(StringArg name)
{
    if(!name) {
        I().name_ = RcString::format("Thread@%p", (void *)this);
    } else {
	    I().name_ = RcString(name);
    }

//	if(I().started()) {
//		pthread_setname_np(I().thd_,I().name_.c_str());
//	}
}
const char *
OsThread::getName() const 
{ 
	return(I().name_.c_str());
}
bool 
OsThread::isAlive() {
    return(I().isAlive());
}

//static unsigned int s_mainThreadId = 0;    

bool
OsThread::isMainThread()
{
//	unsigned int cid = (unsigned int)pthread_self();

//	if(cid == s_mainThreadId) {
//		return(true);
//	}
//	if(!s_mainThreadId) {
//		s_mainThreadId = cid;
//	}
	return(false);
}

int
OsThread::sleep(int32_t milli)
{
    OsThread *ct = _currentOsThread_;
    if(ct == nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds((uint32_t)milli));
    } else if(((uint32_t)milli) < 15) {
        std::this_thread::sleep_for(std::chrono::milliseconds((uint32_t)milli));
        if(ct->interrupted()) {
            return(eWAIT_INTERRUPTED);
        }
    } else {
        return(ct->I().sleep(milli));
    }
    return(0);
}
void
OsThread::yield() {
	std::this_thread::yield();
}
int
OsThread::start(OsThread::ThreadEntry entry, void *arg) {
    return(I().start(entry,arg));
}
int
OsThread::stop() {
    return(I().stop());
}

void 
OsThread::interrupt() {
    I().interrupt();
}

int
OsThread::join( int32_t millis ) {	
    return(I().join(millis));
}
void 
OsThread::detach() {
    I().detach();
}

void 
OsThread::_set_interruptable_waiter_(void *waiter) {
    I().setWaiter(waiter);
}

int 
OsThread::suspend(void)
// resume a suspended thread. Do not call from current thread
{
    return(I().suspend());
}

int 
OsThread::resume(void)
// resume a suspended thread. Do not call from current thread
{
    return(I().resume());
}

ARTD_END 

