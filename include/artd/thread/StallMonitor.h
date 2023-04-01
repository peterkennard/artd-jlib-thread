#ifndef __artd_thread_StallMonitor_h
#define __artd_thread_StallMonitor_h

#include "artd/thread_base.h"
#include "artd/int_types.h"

ARTD_BEGIN

class ARTD_API_JLIB_THREAD StallMonitor
{
    // TODO: do we wish this in a release build ? PK
	#ifdef ARTD_DEBUG
		public:
			StallMonitor();
			~StallMonitor();
		private:
			static uint64_t getMsec();
			bool        isMain_;
			uint64_t    start_;
	#else
		public:
			StallMonitor() { }
	#endif
};

ARTD_END

#endif // __artd_thread_StallMonitor_h
