#ifndef _COMMITSTRATEGY_H_
#define _COMMITSTRATEGY_H_

/*
 * file:	commitStrategy.h
 * purpose:	provides storage commitment algorithm
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */

#include <string>
#include <list>
#include "commit.h"
#include "cstoreutils.h"

using namespace std;

class CommitStrategy
{
public:
	CommitStrategy();
	virtual ~CommitStrategy();
	CommitStrategy(const CommitStrategy& obj);
	CommitStrategy& operator=(const CommitStrategy& obj);

	virtual void commitAlgorithm(vector<COMMIT_ARGS>& commitArgs,
								list<pthread_t>& tids)=0;
	virtual DICOMStoragePkg::CommitType commitType()=0;
};

class SynchronousCommitStrategy : public CommitStrategy {
public:
	SynchronousCommitStrategy();
	~SynchronousCommitStrategy();
	SynchronousCommitStrategy(const SynchronousCommitStrategy& obj);
	SynchronousCommitStrategy& operator=(const SynchronousCommitStrategy& obj);

	void commitAlgorithm(vector<COMMIT_ARGS>& commitArgs,
						list<pthread_t>& tids);
	DICOMStoragePkg::CommitType commitType();
};

class AsynchronousCommitStrategy : public CommitStrategy {
public:
	AsynchronousCommitStrategy();
	~AsynchronousCommitStrategy();
	AsynchronousCommitStrategy(const AsynchronousCommitStrategy& obj);
	AsynchronousCommitStrategy& operator=(const AsynchronousCommitStrategy& obj);

	void commitAlgorithm(vector<COMMIT_ARGS>& commitArgs,
						list<pthread_t>& tids);
	DICOMStoragePkg::CommitType commitType();
};

class EitherCommitStrategy : public CommitStrategy {
public:
	EitherCommitStrategy();
	~EitherCommitStrategy();
	EitherCommitStrategy(const EitherCommitStrategy& obj);
	EitherCommitStrategy& operator=(const EitherCommitStrategy& obj);

	void commitAlgorithm(vector<COMMIT_ARGS>& commitArgs,
						list<pthread_t>& tids);
	DICOMStoragePkg::CommitType commitType();
};

// Storage only strategy without commitment
class NoCommitStrategy : public CommitStrategy {
public:
	NoCommitStrategy();
	~NoCommitStrategy();
	NoCommitStrategy(const NoCommitStrategy& obj);
	NoCommitStrategy& operator=(const NoCommitStrategy& obj);

	void commitAlgorithm(vector<COMMIT_ARGS>& commitArgs,
						list<pthread_t>& tids);
	DICOMStoragePkg::CommitType commitType();
};

#endif
