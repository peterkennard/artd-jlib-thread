#include "artd/OsThread.h"
#include "artd/Logger.h"
#include "artd/pointer_math.h"
#include <windows.h>

ARTD_BEGIN

static int tlsKey_ = -1;
static int mainThreadId_ = -1;

class ARTD_API_JLIB_BASE OsThreadImpl {
    #define inline ARTD_ALWAYS_INLINE  // note all methods here are proxyed to OsThread class
public:

    HANDLE					interruptEvent_;
	void				  * winThread_;
	OsThread::ThreadEntry	entryFunc_;
	void *					entryArg_;

	OsThreadImpl()
	{
		entryArg_ = 0;
		entryFunc_ = 0;

		if(artd::tlsKey_ == -1) {
			artd::tlsKey_ = ::TlsAlloc();
		}

		DWORD id;

		winThread_ = ::CreateThread(0, // LPSECURITY_ATTRIBUTES lpsa, 
							0, // DWORD cbStack, 
							threadProc, 
							(LPVOID)this, // LPVOID lpvThreadParam, 
							CREATE_SUSPENDED, // DWORD fdwCreate, CREATE_SUSPENDED
							&id // LPDWORD lpIDThread
							); 

		if(winThread_ == 0) {
			AD_LOG(debug) << "init failure";
			myBase()->flags_ = (OsThread::thSTOPPED | OsThread::thCLEANEDUP);
			return;
		}

		interruptEvent_ = ::CreateEvent(  NULL,         // default security attributes
											false,        // auto-reset event
											false,        // initial state is *NOT* signaled
											NULL          // Unamed
										); 
        
		myBase()->flags_ = (OsThread::thINITIALIZING | OsThread::thALIVE | OsThread::thSUSPENDED);
	}

	~OsThreadImpl() {
		// LOGDEBUG("destroying OSThreadImpl");
		if(interruptEvent_) {
			::CloseHandle(interruptEvent_);
			interruptEvent_ = 0;
		}
	}

	inline OsThread *myBase() {
        return(ArtdToClass(OsThread,impl_,this)); 
    }
	
	inline int32_t getFlags(int32_t flags) {
        return(myBase()->getFlags(flags));
    }
    inline void setFlags(int32_t flags) {
        myBase()->setFlags(flags);
    }
    inline void clearFlags(int32_t flags) {
        myBase()->clearFlags(flags);
    }
	
	void run() {
		::TlsSetValue(tlsKey_,myBase()); // in WinBase.h    	
		try {
			setFlags(OsThread::thALIVE);
			clearFlags(OsThread::thINITIALIZING | OsThread::thSUSPENDED);
			if(entryFunc_) {
				(*entryFunc_)(entryArg_);
			}
		} catch(...) {
			AD_LOG(debug) << "unhandled C++ exception in thread " << ((void *)myBase());
		}
	}

	static DWORD WINAPI threadProc(void far *arg) {
		__try {		
			((OsThreadImpl *)arg)->run();
		} __finally {
		    ((OsThreadImpl *)arg)->setFlags(OsThread::thSTOPPED);     
			((OsThreadImpl *)arg)->cleanup();
			::ExitThread(0);		
		}
		return(0);	
	}

	inline int start(OsThread::ThreadEntry entry, void *arg) {

		if(!(getFlags(OsThread::thINITIALIZING))) {
			AD_LOG(debug) << "failure STARTING THREAD";
			return(-1);
		}
		clearFlags(OsThread::thINITIALIZING);

		entryFunc_ = entry;
		entryArg_ = arg;
		
		if(::ResumeThread(winThread_) < 0) {
			AD_LOG(debug) << "failure RESUMING THREAD";
			return(-1);
		}
		return(0);
	}

	void cleanup()
	{
		if(!(getFlags(OsThread::thCLEANEDUP)))
		{
			setFlags(OsThread::thCLEANEDUP | OsThread::thSTOPPED);
			clearFlags(~(OsThread::thCLEANEDUP | OsThread::thSTOPPED));
			myBase()->onStop();
		}
	}


#if 0
	/** returns WAIT_INTERRUPTED if thread was interrupted */
	static int 
	waitForSingleObject(HANDLE obj, int millis)
	{
		OsThread *t = OsThread::currentOsThread();
		if(t) {
			HANDLE h[2];
			h[0] = t->I().interruptEvent_;
			h[1] = obj;

			int ret = ::WaitForMultipleObjects(2,h,false,millis);
			if(ret == 1) {
				return(0);
			}
			if(ret == 0) {
				return(eWAIT_INTERRUPTED);
			}
			return(ret);

		} else {
			return(::WaitForSingleObject(obj,millis));
		}
	}
#endif // 0

	inline static void	onDllDetach() {
		OsThread *th = OsThread::currentOsThread();
		if(th) {
			th->I().cleanup();
		}
	}

	#undef inline
};


OsThread::OsThread(StringArg name)
{
	ARTD_STATIC_ASSERT((sizeof(OsThreadImpl) <= sizeof(impl_)));
	new(&I()) OsThreadImpl();
}
OsThread::~OsThread() {
	// LOGDEBUG("destroying OSThread");
	I().~OsThreadImpl();
}

int
OsThread::start(OsThread::ThreadEntry entry, void *arg) {
    return(I().start(entry,arg));
}

bool
OsThread::isMainThread() {
	return(mainThreadId_ == ::GetCurrentThreadId());
}

int32_t 
OsThread::currentThreadId()
{
    ARTD_STATIC_ASSERT(sizeof(int32_t) == sizeof(mainThreadId_));
	return(::GetCurrentThreadId());
}

OsThread *
OsThread::currentOsThread() {
	return((OsThread *)::TlsGetValue(tlsKey_)); // in WinBase.h
}

void 
OsThread::yield() { 
	::Sleep(0); 
}

int 
OsThread::sleep(int32_t millis)
{
	OsThread *t = OsThread::currentOsThread();
	if(t) {
		if(::WaitForSingleObject(t->I().interruptEvent_, millis) == 1)
			return(eWAIT_INTERRUPTED);
		return(0);
	} else {
		::Sleep(millis);
		return(0);
	}
}

void 
OsThread::interrupt()
{ 
	setFlags(OsThread::thINTERRUPTED);
	if(I().getFlags(thALIVE|thINITIALIZING|thSUSPENDED) != 0) {	  
		::SetEvent(I().interruptEvent_);
	}
}

int 
OsThread::resume(void)
// resume a suspended thread. Do not call from current thread
{
	uint32_t n = 0;

	// this has some timing quirks in the suspend flag needs to be set before 
	// since when a thread is deleting itself then it may do so inside the Resume
	// making the flag alter already remapped memory. 

	if((getFlags(thSUSPENDED | thALIVE | thINITIALIZING | thCLEANEDUP)) == (thSUSPENDED | thALIVE))
	{     
		clearFlags(thSUSPENDED);
		while ((n = ::ResumeThread(I().winThread_)) > 1) 
		{
			if (n < 0)  // error condintion
				break;
		}
	}
	return(n);
}

int 
OsThread::suspend(void)
{
	if(getFlags(thSUSPENDED | thCLEANEDUP))
		return(0);

	setFlags(thSUSPENDED);
	// Need to Fix for Windows errors
	return(::SuspendThread(I().winThread_));
}
bool 
OsThread::isAlive() 
{ 
	return(this && getFlags(thALIVE|thINITIALIZING|thSUSPENDED)); 
}
int 
OsThread::join(int32_t timeout)
{
	// LOGDEBUG("joining %p %d", this, unsafeGetRefCount());


	// TODO: make this better ot use boost XThread
	// for now only one thing can be waiting on join
	/*
	if(!isAlive()) {
		return(0);
	}
	Semaphore		dieEvent;
	O().sema_ =		&dieEvent;

	if(dieEvent.timed_wait(timeout)) {
		O().sema_ = 0;
		// LOGDEBUG("quit signalled %p", this);
		return(0);
	}
	O().sema_ = 0;
	*/

	while(isAlive()) 
	{
		if(timeout > 5) {
			::Sleep(5);
			timeout -= 5;
		} else if(timeout <= 0) {
			if(timeout < 0) {
				::Sleep(5);
				continue;
			}
			return(eWAIT_TIMEOUT);
		} else {
			::Sleep(timeout);
			timeout = 0;
		}
	}

//    LOGDEBUG("join complete %p %d", this, unsafeGetRefCount());

	return(0); 
}

void * 
OsThread::_get_interruptable_data_() {
    return((void *)(I().interruptEvent_));
}


int 
OsThread::stop()
{ 
	if(I().winThread_) 
	{
		if(currentOsThread() == this) {
			I().winThread_ = 0;
			::ExitThread(0);
		} else { 
			::TerminateThread(I().winThread_,0);
			I().winThread_ = 0;
		}
	}
	return(0);
}

/*
void * 
OsThread::currentInterruptEvent()
{
	Thread *t = currentThread();
	if(t) {
		return(t->I().interruptEvent_);
	}
	return(0);
}
*/

ARTD_END

BOOLEAN WINAPI DllMain( HINSTANCE hDllHandle, 
						DWORD     nReason, 
						LPVOID    Reserved )
{
	BOOLEAN bSuccess = TRUE;

	switch ( nReason ) {
		case DLL_PROCESS_ATTACH:
			//DisableThreadLibraryCalls( hDllHandle );
			artd::tlsKey_ = ::TlsAlloc();
			artd::mainThreadId_ = ::GetCurrentThreadId();
			break;
		case DLL_PROCESS_DETACH:
			break;
		case DLL_THREAD_ATTACH:
			// LOGDEBUG("thread attach");
			break;
		case DLL_THREAD_DETACH:
			// LOGDEBUG("thread detach");
			artd::OsThreadImpl::onDllDetach();
			break;
	}
	return(bSuccess);
}

