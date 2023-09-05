/*-
 * Copyright (c) 1991-2022 Peter Kennard and aRt&D Lab
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
 *  $Id: Thread.cpp 60 2009-02-14 23:16:59Z peterk $
 */

#define __artd_Thread_cpp

#include "artd/Thread.h"
#include "artd/Semaphore.h"
#include "artd/RcString.h"
#include "artd/Logger.h"

ARTD_BEGIN

Thread::Thread(StringArg name)
	: OsThread(name), toRun_{nullptr,nullptr}
{
	hackToRun().setObj(this);
}
Thread::Thread(ObjectPtr<Runnable> r, StringArg name)
	: Thread(name)
{
	
	if (sameOwner(r)) {
		hackToRun().setObj(r.get());
		hackToRun().setCb(nullptr);
	} else {
		// AD_LOG(debug) << "new Thread " << getName() << " referencing toRun");
		new(&toRun()) ObjectPtr<Runnable>(std::move(r));
	}
}

Thread::~Thread()
{
	AD_LOG(info) << "Thread " << getName() << " being destroyed";
	stop();
	if (hackToRun().cbPtr() != nullptr) {
		toRun() = nullptr; // dereference
	}
}

void 
Thread::onStop()
{
	AD_LOG(debug) << "thread " << getName() << " stopped " << toRun().use_count();

	if (toRun().get() != static_cast<Runnable*>(this)) {
		toRun() = nullptr;
	}
	this->release(); // this now does test below
}

void
Thread::_entryFunc_(void *arg) {
	// AD_LOG(debug) << "in Thread entry func");
	Thread* me = ((Thread*)arg);
	AD_LOG(debug) << "thread " << me->getName() << " started " << me->toRun().use_count();
	me->toRun().get()->run();
}

int
Thread::start() {

	this->addRef();
	int ret = super::start(_entryFunc_, this);
	if (ret != 0) {
		this->release();
	}
	return(ret);
}

int
Thread::stop() {
	return(super::stop());
}

ARTD_END


