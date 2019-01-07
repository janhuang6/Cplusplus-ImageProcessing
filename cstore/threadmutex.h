/*
 * file: $Source: /usr/adac/Repository/camera/camera/binner/src/libacqbase/threadmutex.h,v $
 *
 * inspection history:
 *
 * ----------------------------
 * Source Code pre-history
 * revision 1.1
 * Modification Request Number from DDT:           PEGma16408
 * Reason for changes. (OK to use several lines):  Adding threading classes
 * to enable the thread synchronization changes in the control server.
 * They are added here to support their future use in other sub-systems.
 * ----------------------------
 *
 * $Id: threadmutex.h,v 1.4 Exp $
 */
#include <pthread.h>

#ifndef _THREADMUTEX_H_
#define _THREADMUTEX_H_

/**
 * A simple mutex class that can be managed by the
 * MutexGuard.  While the ThreadMutex can be recursive,
 * an instance of a MutexGuard cannot reacquire the lock,
 * another MutexGuard in the same thread is free to re-acquire
 * the lock
 */
class ThreadMutex {
public:
	enum ThreadMutexFlags {
		DEFAULT=		0x0000,
		ERROR_CHECKING=		0x0001,
		RECURSIVE=		0x0002,
		// include combinations so we can pass as legal enum!!
		__DUMMY003=		0x0003
	};
		
	ThreadMutex( ThreadMutexFlags flags = DEFAULT );
	~ThreadMutex();
	bool isReady();
	friend class MutexGuard;
private:
	pthread_mutex_t _mutex;
	volatile bool _ready;

	// Disallow copying and assignment
	ThreadMutex(const ThreadMutex &);
	void operator=(const ThreadMutex &);
};

class MutexGuard {
public:
	MutexGuard(ThreadMutex &);
	~MutexGuard();

	void release();
private:
	ThreadMutex *_lock;
	bool _owner;

	void acquire();

	// Disallow copying and assignment
	MutexGuard(const MutexGuard &);
	void operator= (const MutexGuard &);
};

/*
 * The isReady method returns the initialization state 
 */

inline bool ThreadMutex::isReady() {
	return _ready;
}

#endif /* _THREADMUTEX_H_ */
