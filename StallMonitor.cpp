#include "artd/thread/StallMonitor.h"
#include "artd/thread/OsThread.h"
#include "artd/Logger.h"

#ifdef ARTD_WINDOWS
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN		/// &brief Exclude rarely-used stuff from Windows headers
	#endif
	#include "Windows.h"
	#include <mmsystem.h>
    #pragma comment( lib, "winmm.lib")   
#else
	#include <time.h>
#endif
#include <stdio.h>


ARTD_BEGIN

#if 0 // TODO: move to a test module
#include <iostream>
#include <chrono>

// long operation to time
long long fib(long long n) {
	if (n < 2) {
		return n;
	}
	else {
		return fib(n - 1) + fib(n - 2);
	}
}

int main() {
	auto start_time = std::chrono::high_resolution_clock::now();

	long long input = 32;
	long long result = fib(input);

	auto end_time = std::chrono::high_resolution_clock::now();
	auto time = end_time - start_time;

	std::cout << "result = " << result << '\n';
	std::cout << "fib(" << input << ") took " <<
		time / std::chrono::milliseconds(1) << "ms to run.\n";
}
#endif // 0


#ifdef ARTD_DEBUG

uint64_t StallMonitor::getMsec()
{
	#ifdef ARTD_WINDOWS
		
		// auto time = std::chrono::high_resolution_clock::now();
		// return(time / std::chrono::milliseconds(1));
		
	     return((uint64_t)timeGetTime());
	#else		
		#ifdef ARTD_IOS  // TODO: BSD defines
			clock_t res = clock();
			return(res / (CLOCKS_PER_SEC/1000LL));
		#else // ARTD_LINUX
			struct timespec ts;
			int res = clock_gettime(CLOCK_REALTIME, &ts);
			ARTD_ASSERT(!res);
			uint64_t nsec = (uint64_t)ts.tv_nsec;
			nsec += (uint64_t) ts.tv_sec * 1000000000;
			return nsec / 1000000;
		#endif
	#endif
}

StallMonitor::StallMonitor()
{
	#ifdef ARTD_IOS  // TODO: make work for iOS
		isMain_ = false;
	#else
		if((isMain_ = OsThread::isMainThread())) {
			start_ = getMsec();
		}
	#endif
}

StallMonitor::~StallMonitor()
{
	if(isMain_)
	{
		int64_t deltaMsec = (int64_t)(getMsec() - start_);
		if (deltaMsec > 100) {
			AD_LOG(warning) << "*** MAIN THREAD STALL, " << deltaMsec << "  msec ***";
		}
	}
}

#endif // ARTD_DEBUG

ARTD_END
