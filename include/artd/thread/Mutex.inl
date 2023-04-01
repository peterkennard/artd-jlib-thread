// only to be included by artd/Mutex.h

#pragma warning( push )
#pragma warning( disable : 26110)

#define INL ARTD_ALWAYS_INLINE

INL ::artd::Mutex::Mutex() {}
INL ::artd::Mutex::~Mutex() {}

INL void ::artd::Mutex::acquire() {
    impl_.lock();
#ifdef ARTD_DEBUG
     lockCount_++;
#endif
}

INL void ::artd::Mutex::release() {
#ifdef ARTD_DEBUG
	ARTD_ASSERT(lockCount_); // attempt to release a mutex that isn't locked
	lockCount_--;
#endif
	impl_.unlock();
}

INL bool ::artd::Mutex::tryAcquire() {
	bool res = impl_.try_lock();
#ifdef ARTD_DEBUG
	if (res)
		lockCount_++;
#endif
	return res;
}

#pragma warning( pop )


#undef INL