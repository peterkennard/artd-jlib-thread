/*-
 * Copyright (c) 1995-2011 Peter Kennard and aRt&D Lab
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of the source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Any redistribution solely in binary form must conspicuously
 *    reproduce the following disclaimer in documentation provided with the
 *    binary redistribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'', WITHOUT ANY WARRANTIES, EXPRESS
 * OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  LICENSOR SHALL
 * NOT BE LIABLE FOR ANY LOSS OR DAMAGES RESULTING FROM THE USE OF THIS
 * SOFTWARE, EITHER ALONE OR IN COMBINATION WITH ANY OTHER SOFTWARE.
 *
 * 	$Id$
 */
#ifndef __artd_synchronized_h
#define __artd_synchronized_h

#include <artd/jlib_base.h>



/*******************************************************************
 * Implementation of java-like "synchronized" functionality that uses
 * any objects exposing "acquire()" and "synchro_release()" methods
 *
 */

#include <artd/Mutex.h>

ARTD_BEGIN

class SynchroLock {

private:

	void *plock_;
	void (*release_)(void *);

	template< class T >
	static void _release_(void *pl)
	{
		reinterpret_cast<T *>((char *)pl)->synchro_release();
	}


public:

	template<class LockT>
	ARTD_ALWAYS_INLINE SynchroLock(LockT &lp) 
		: plock_(&lp)
		, release_(&_release_<LockT>)
	{
		lp.acquire(); 
	}
	template<class LockT>
	ARTD_ALWAYS_INLINE SynchroLock(LockT *lp) 
		: plock_(lp)
		, release_(&_release_<LockT>)
	{
		lp->acquire();
	}

	ARTD_ALWAYS_INLINE ~SynchroLock() 
	{ 
		release_(plock_); 
	}
};


#define ARTD_synchronizer0_(_mux,suf) ::artd::SynchroLock _sync##suf(_mux);
#define ARTD_synchronizer_(_mux,_suf) ARTD_synchronizer0_(_mux,_suf)

#define synchronized(_mux) ARTD_synchronizer_(_mux,__COUNTER__)

/*
 * use synchronized like this to lock section between { and }
 *
 *  {
 *  synchronized(this);  // argument is object containing CriticalSection
 *      executed();
 *      inside();
 *      lock();
 *  }
 */

ARTD_END

#endif // __artd_synchronized_h
