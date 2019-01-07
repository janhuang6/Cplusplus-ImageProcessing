#ifndef _COMMIT_H_
#define _COMMIT_H_

/*
 * file:	commit.h
 * purpose:	Commit method class that commits a list of DICOM files
 *          from one Target
 * created:	17-Oct-07
 * property of:	Philips Nuclear Medicine
 *
 * revision history:
 *   17-Oct-07	Jiantao Huang		    initial version
 */

#include "cstoreutils.h"

struct CommitConfig
{
	DICOMStoragePkg::DICOMTarget reportingSystem;
	int                          roleReversalWaitTime; // in seconds
};

/*
 * Commit base class
 */

class Commit
{
public:
	Commit();
	virtual ~Commit() throw ();
	Commit(const Commit& obj);
	Commit& operator=(const Commit& obj);

	virtual void commit(vector<COMMIT_ARGS>& commitArgs,
						list<pthread_t>& tids) = 0;
};

/*
 * Synchronous Commit class
 */

class SynchCommit : public Commit
{
public:
	SynchCommit();
	virtual ~SynchCommit() throw ();
	SynchCommit(const SynchCommit& obj);
	SynchCommit& operator=(const SynchCommit& obj);

	void commit(vector<COMMIT_ARGS>& commitArgs,
				list<pthread_t>& tids);
};

/*
 * Asynchronous Commit class
 */

class AsynchCommit : public Commit
{
public:
	AsynchCommit();
	virtual ~AsynchCommit() throw ();
	AsynchCommit(const AsynchCommit& obj);
	AsynchCommit& operator=(const AsynchCommit& obj);

	void commit(vector<COMMIT_ARGS>& commitArgs,
				list<pthread_t>& tids);
};

/*
 * Either Commit class
 */

class EitherCommit : public Commit
{
public:
	EitherCommit();
	virtual ~EitherCommit() throw ();
	EitherCommit(const EitherCommit& obj);
	EitherCommit& operator=(const EitherCommit& obj);

	void commit(vector<COMMIT_ARGS>& commitArgs,
				list<pthread_t>& tids);
};

#endif
