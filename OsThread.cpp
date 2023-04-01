#define __artd_OsThread_cpp

#include "artd/thread/OsThread.h"

ARTD_BEGIN
	thread_local OsThread *_currentOsThread_ = nullptr;
	OsThread* OsThread::currentOsThread() {
		return(_currentOsThread_);
	}
ARTD_END

#if defined(ARTD_THREAD_STD)
	#include "./OsThreadStd.cpp.h"
#elif defined(ARTD_THREAD_PTHREAD)
    #include "./OsThreadWindows.cpp.h"
#elif defined(ARTD_THREAD_WINDOWS)
    #include "./OsThreadWindows.cpp.h"
#endif




