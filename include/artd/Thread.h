///*-
// * Copyright (c) 1991-2008 Peter Kennard and aRt&D Lab
// * All rights reserved.
// * 
// * Redistribution and use in source and binary forms, with or without
// * modification, are permitted provided that the following conditions are
// * met:
// * 
// * 1. Redistributions of the source code must retain the above copyright
// *    notice, this list of conditions and the following disclaimer.
// * 
// * 2. Any redistribution solely in binary form must conspicuously
// *    reproduce the following disclaimer in documentation provided with the
// *    binary redistribution.
// * 
// * THIS SOFTWARE IS PROVIDED ``AS IS'', WITHOUT ANY WARRANTIES, EXPRESS
// * OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, WARRANTIES OF
// * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  LICENSOR SHALL
// * NOT BE LIABLE FOR ANY LOSS OR DAMAGES RESULTING FROM THE USE OF THIS
// * SOFTWARE, EITHER ALONE OR IN COMBINATION WITH ANY OTHER SOFTWARE.
// * 
// *  $Id: Thread.h 60 2009-02-14 23:16:59Z peterk $
// */

#ifndef __artd_Thread_h
#define __artd_Thread_h

#include "artd/ObjectBase.h"
#include "artd/thread/OsThread.h"
#include "artd/Logger.h"

#ifdef WINAPI

extern "C" {
    BOOL APIENTRY     DllMain( HANDLE hModule, 
                               DWORD  ul_reason_for_call, 
                               LPVOID lpReserved);				 
}

#endif


ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

class Semaphore;

class ARTD_API_JLIB_THREAD Runnable
    : public ObjectBase
{
public:
    virtual void run() = 0;
};


class ARTD_API_JLIB_THREAD Thread
    : public Runnable
    , public OsThread // must be first as this is an "ObjectBase"
{
private:
    static void _entryFunc_(void *arg);
    typedef OsThread super;

protected:
	// override this ( virtual in OsThread) 
	// for special cleanup operations ie: windows has a peculiar protocol.
	void onStop();

public:
    #ifdef __EMSCRIPTEN__
       #pragma GCC diagnostic ignored "-Wkeyword-macro"
    #endif

    Thread(ObjectPtr<Runnable> r, StringArg name = 0);
    Thread(StringArg name=0);
    ~Thread();

    int				    stop();
    int				    start();

	// @Override Runnable
    virtual void	        run() override {};

	INL bool			isMainThread() { return(super::isMainThread()); }
	INL static Thread *	currentThread() { return(static_cast<Thread*>(super::currentOsThread())); }
	INL static id_t currentThreadId() { return(super::currentThreadId()); }
	INL static int		sleep(int32_t millis = -1) { return(super::sleep(millis)); }
	INL static void		yield() { super::yield(); }
	INL int				suspend(void) { return(super::suspend()); }
	INL int				resume(void) { return(super::resume()); }
	INL bool			isAlive() { return(super::isAlive()); }
    // set the state to interrupted.
	INL void			interrupt() { super::interrupt(); }
    // tests state of interrupted flag and clears it 
	INL bool            interrupted() { return(super::interrupted()); }
    // tests state of interrupted flag and leaves it set as it was.
	INL bool            isInterrupted() { return(super::isInterrupted()); }
	INL int				join(int32_t timeoutMillis = -1) { 
        ObjectPtr<Runnable> ref;
        sharedFromThis<Runnable>((Runnable*)this);
        return(super::join(timeoutMillis)); 
    }

private:
    HackStdShared<Runnable> toRun_;
    INL ObjectPtr<Runnable>& toRun() {
        return(toRun_.objptr());
    }
};

#undef INL

ARTD_END



#endif // __artd_Thread_h
