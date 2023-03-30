#include "artd/jlib-base"

// TODO: we need to handle this for building either static or dll libraries !!

#ifndef ARTD_API_JLIB_THREAD
    #ifdef BUILDING_artd_jlib_thread
        #define ARTD_API_JLIB_THREAD ARTD_SHARED_LIBRARY_EXPORT
    #else
        #define ARTD_API_JLIB_THREAD ARTD_SHARED_LIBRARY_IMPORT
    #endif
#endif