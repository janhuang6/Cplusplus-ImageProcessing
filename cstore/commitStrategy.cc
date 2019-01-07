/*
 * file:	storageStrategy.cc
 * purpose:	Implementation of the storage commitment algorithm
 * created:	08-Nov-07
 * property of:	Philips Nuclear Medicine
 *
 * revision history:
 *   08-Nov-07	Jiantao Huang		    initial version
 */

#include "commitStrategy.h"

/*
 * CommitStrategy class
 */

CommitStrategy::CommitStrategy()
{
}

CommitStrategy::~CommitStrategy()
{
}

CommitStrategy::CommitStrategy(const CommitStrategy& obj)
{
}

CommitStrategy& CommitStrategy::operator=(const CommitStrategy& obj)
{	return *this;
}

/*
 * SynchronousCommitStrategy class
 */

SynchronousCommitStrategy::SynchronousCommitStrategy()
{
}

SynchronousCommitStrategy::~SynchronousCommitStrategy()
{
}

SynchronousCommitStrategy::SynchronousCommitStrategy(const SynchronousCommitStrategy& obj)
					: CommitStrategy(obj)
{
}

SynchronousCommitStrategy& SynchronousCommitStrategy::operator=(const SynchronousCommitStrategy& obj)
{
	CommitStrategy:: operator=(obj);
	return *this;
}

void SynchronousCommitStrategy::commitAlgorithm(vector<COMMIT_ARGS>& commitArgs,
						list<pthread_t>& tids)
{	
	SynchCommit synchCommit;
	synchCommit.commit(commitArgs, tids);
}

DICOMStoragePkg::CommitType SynchronousCommitStrategy::commitType()
{
	return DICOMStoragePkg::SYNCH_COMMIT;
}

/*
 * AsynchronousCommitStrategy class
 */

AsynchronousCommitStrategy::AsynchronousCommitStrategy()
{
}

AsynchronousCommitStrategy::~AsynchronousCommitStrategy()
{
}

AsynchronousCommitStrategy::AsynchronousCommitStrategy(const AsynchronousCommitStrategy& obj)
					: CommitStrategy(obj)
{
}

AsynchronousCommitStrategy& AsynchronousCommitStrategy::operator=(const AsynchronousCommitStrategy& obj)
{
	CommitStrategy:: operator=(obj);
	return *this;
}

void AsynchronousCommitStrategy::commitAlgorithm(vector<COMMIT_ARGS>& commitArgs,
						list<pthread_t>& tids)
{	
	AsynchCommit asynchCommit;
	asynchCommit.commit(commitArgs, tids);
}

DICOMStoragePkg::CommitType AsynchronousCommitStrategy::commitType()
{
	return DICOMStoragePkg::ASYNCH_COMMIT;
}

/*
 * EitherCommitStrategy class
 */

EitherCommitStrategy::EitherCommitStrategy()
{
}

EitherCommitStrategy::~EitherCommitStrategy()
{
}

EitherCommitStrategy::EitherCommitStrategy(const EitherCommitStrategy& obj)
					: CommitStrategy(obj)
{
}

EitherCommitStrategy& EitherCommitStrategy::operator=(const EitherCommitStrategy& obj)
{
	CommitStrategy:: operator=(obj);
	return *this;
}

void EitherCommitStrategy::commitAlgorithm(vector<COMMIT_ARGS>& commitArgs,
						list<pthread_t>& tids)
{	
	EitherCommit eitherCommit;
	eitherCommit.commit(commitArgs, tids);
}

DICOMStoragePkg::CommitType EitherCommitStrategy::commitType()
{
	return DICOMStoragePkg::EITHER_COMMIT;
}

/*
 * NoCommitStrategy class
 */

NoCommitStrategy::NoCommitStrategy()
{
}

NoCommitStrategy::~NoCommitStrategy()
{
}

NoCommitStrategy::NoCommitStrategy(const NoCommitStrategy& obj)
					: CommitStrategy(obj)
{
}

NoCommitStrategy& NoCommitStrategy::operator=(const NoCommitStrategy& obj)
{
	CommitStrategy:: operator=(obj);
	return *this;
}

void NoCommitStrategy::commitAlgorithm(vector<COMMIT_ARGS>& commitArgs,
						list<pthread_t>& tids)
{	// There is nothing to do
	tids.clear();
}

DICOMStoragePkg::CommitType NoCommitStrategy::commitType()
{
	return DICOMStoragePkg::NO_COMMIT;
}
