#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include "WaitableSignal.h"

ARTD_BEGIN

class ARTD_API_ARTD_SYS OsThreadImpl {
public:
	static const pthread_t invalidThread = 0;

	RcString		name_;
	pthread_t    thd_;
	bool			    exitedCleanly_;
	bool			    started_;
	
	OsThreadImpl() : thd_(invalidThread), exitedCleanly_(false) 
	{}
	~OsThreadImpl();
	static void setCurrentThreadName(const char *name);
	bool        started() const;
	bool        join( int32_t millis );
};


ARTD_ALWAYS_INLINE OsThreadImpl::~OsThreadImpl()
{
	if (pthread_t  t = thd_) 
	{
		thd_ = invalidThread;
	#ifdef VALIDATE_THREAD_DELETION
	//	ARTD_ASSERT(m_exitedCleanly);
	#endif
		if(t  != invalidThread) {
			// pthread_kill(t,  );
		}
	}
}
bool 
OsThreadImpl::started() const
{
	return(thd_ != invalidThread);
}

int
ARTD_ALWAYS_INLINE OsThreadImpl::join( int32_t mills )
{
	//LOGTRACE( "argh, wait not implemented\n" );
//	if (boost::this_thread::get_id() == thd_->get_id())
//	{
//		// attempting to join current thread implicitly detaches it
//		thd_->detach();
//		delete thd_;
//		thd_ = 0;
//		return true;
//	}

/*
void f(int id)
{
    {
    std::lock_guard<std::mutex> _(m);
    ++thread_count;
    }
    while (true)
    {
        {
        std::lock_guard<std::mutex> _(m);
        std::cout << "thread " << id << " working\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        std::lock_guard<std::mutex> _(m);
        if (time_to_quit)
            break;
    }
    std::lock_guard<std::mutex> _(m);
    std::cout << "thread ended\n";
    --thread_count;
    cv.notify_all();
}
*/


    if (mills == -1) {
		thd_->join();
		return(0);
    }
//	if ( !thd_->timed_join( boost::posix_time::milliseconds(mills) ) ) {
//		thd_->detach();
//		return false;
//	}
	//no imp
	return true;
}
void OsThreadImpl::setCurrentThreadName(const char *szThreadName)
{
}

OsThread::OsThread(StringArg name)
{
	ARTD_STATIC_ASSERT((sizeof(OsThreadImpl) <= sizeof(impl_)));
	new(&I()) OsThreadImpl();
	setName(name);
}
OsThread::~OsThread()
{
	I().~OsThreadImpl();
}
unsigned int 
OsThread::getCurrentThreadId()
{
	unsigned int id = pthread_self();
	return (unsigned int) id;
}
void 
OsThread::setName(StringArg name)
{
	I().name_ = name;
	if(I().started()) {
		pthread_setname_np(I().thd_,I().name_.c_str());
	}
}
const char *
OsThread::getName() const 
{ 
	return(I().name_.c_str()); 
}

static unsigned int s_mainThreadId = 0;    

bool
OsThread::isMainThread()
{
	unsigned int cid = (unsigned int)pthread_self();

	if(cid == s_mainThreadId) {
		return(true);
	}
	if(!s_mainThreadId) {
		s_mainThreadId = cid;
	}
	return(cid == s_mainThreadId);
}

int 
OsThread::sleep(int32_t milli)
{
	usleep( milli * 1000 );
    return(0);  // WAIT_INTERRUPTED
  // WAIT_TIMEOUT // errno.h ETIMEDOUT
}
void
OsThread::yield() {
	usleep( 1 );
}

class PThreadEntryWrapper
	:  public Object
{
public:

	OsThread::ThreadFunc    entry_;
	void*                                   arg_;
	OsThread *                         ost_;
	WaitableSignal                  startSignal_;

	PThreadEntryWrapper( OsThread::ThreadFunc entry, void* arg, OsThread *ost) : entry_(entry), arg_(arg), ost_(ost) {}
	static void *entryPoint(void *arg)
	{
		PThreadEntryWrapper *ew = (PThreadEntryWrapper *)arg;
		// OsThreadImpl::setCurrentThreadName(ew->ost_->getName());
		// ew->startSignal_.signal();
		
		OsThread::ThreadFunc entry = ew->entry_;
        OsThread *ost = ew->ost_;
		arg = ew->arg_;
        // no more references to original argument after this.
		if (entry) {
			(*(entry))(arg);
		}
		ost->I().exitedCleanly_ = true;
		return(0);
	}
};


bool
OsThread::start(OsThread::ThreadFunc entry, void *arg ) {
	if ( entry == NULL ) return false;
	if ( I().started()) {
		ARTD_ASSERT(0 && "OsThread: thread already started\n" );
		return false;
	}
	isMainThread(); // will set main thread ID
	ObjectPtr<PThreadEntryWrapper> osw = NewObject<PThreadEntryWrapper>(entry, arg, this); 
	pthread_attr_t startAtt;
	pthread_attr_init(&startAtt); // should we have one static one for all threads ?
	int ret = pthread_create(&I().thd_, &startAtt, &PThreadEntryWrapper::entryPoint, osw.operator->());
	pthread_attr_destroy(&startAtt);
	if(ret != 0) {
		I().thd_ = OsThreadImpl::invalidThread;
		return false;	
	}
	return(osw->startSignal_.waitOnSignal(2000));
}

OsThread *
OsThread::currentOsThread() {
    return(NULL); // for now
}

bool 
OsThread::join( int32_t millis )
{
	if ( !I().started()) 
		return(true);
	return(I().join(millis));
}
void 
OsThread::detach()
{
//	if ( !I().started() ) return;
//	I().thd_->detach();
//	delete I().thd_;
//	I().thd_ = 0;
//	#ifdef VALIDATE_THREAD_DELETION
//		I().exitedCleanly_ = true;
//	#endif
}


ARTD_END // __artd_OsThread_h

