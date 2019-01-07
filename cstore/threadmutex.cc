/*
 * file: $Source: /usr/adac/Repository/camera/camera/binner/src/libacqbase/threadmutex.cc,v $
 * Copyright (c) 2002 by ADAC Laboratories (a Philips Medical Systems Company)
 * Confidential - All Rights Reserved
 *
 * These classes are based on examples in the book
 * "Multi-Threaded Programming in C++" by Mark Walmsley, Springer, 2000
 * and the book "Pattern-Oriented Software Architecture, Volume 2"
 * by Douglas Schmidt, et al, Wiley, 2000
 *
 * inspection history:
 *
 * ----------------------------
 * Source Code pre-history
 * revision 1.1
 * date: 2002/07/08 03:50:59;  author: ccarman;  state: Exp;
 * Modification Request Number from DDT:           PEGma16408
 * Who authorized this change (use e-mail name):   ccarman
 * Reason for changes. (OK to use several lines):  Adding threading classes
 * to enable the thread synchronization changes in the control server.
 * They are added here to support their future use in other sub-systems.
 * ----------------------------
 *
 * $Id: threadmutex.cc,v 1.3 2006/12/20 02:04:41 sheavner Exp $
 */
#include <sched.h>
#include <assert.h>
#include "threadmutex.h"

/*
 * The default ThreadMutex constructor initializes the mutex
 */

ThreadMutex::ThreadMutex( ThreadMutexFlags flags ) {
	pthread_mutexattr_t attrib;
	int status;
	_ready = false;
	status = pthread_mutexattr_init(&attrib);
	assert(status == 0);
	if ( flags & ERROR_CHECKING ) {
		status = pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_ERRORCHECK);
		assert(status == 0);
	}
	if ( flags & RECURSIVE ) {
		status = pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_RECURSIVE);
		assert(status == 0);
	}
	if (0 == pthread_mutex_init(&_mutex, &attrib)) {
		_ready = true;
	}
	pthread_mutexattr_destroy(&attrib);
}

/*
 * This ThreadMutex destructor destroys the mutex
 */

ThreadMutex::~ThreadMutex() {
	int status;
	/* first make sure that the mutex is not locked */
	status = pthread_mutex_lock(&_mutex);
	assert(status == 0);
	_ready = false;
	status = pthread_mutex_unlock(&_mutex);
	assert(status == 0);
	sched_yield();

	pthread_mutex_destroy(&_mutex);
}

/*
 * The MutexGuard default constructor acquires a lock on the guard mutex
 */

MutexGuard::MutexGuard( ThreadMutex &lock ) : _lock(&lock), _owner(false) {
	assert(_lock->isReady());
	acquire();
}

/*
 * The MutexGuard destructor releases the lock on the guard mutex
 */

MutexGuard::~MutexGuard() {
	// Only release the lock if we are the owner
	if ( true == _owner && _lock->isReady() )
		release();
}

/*
 * The acquire method blocks until it acquires a lock on the mutex
 */

void MutexGuard::acquire() {
	if ( true == _owner ) 
		return;
	assert(_lock->isReady());
	int status = pthread_mutex_lock(&(_lock->_mutex));
	assert(status == 0);
	_owner = true;
}

/*
 * The release method releases the lock on the mutex
 */

void MutexGuard::release() {
	if ( false == _owner || ! _lock->isReady() ) return;

	int status = pthread_mutex_unlock(&(_lock->_mutex));
	assert(status == 0);
	_owner = false;
}

