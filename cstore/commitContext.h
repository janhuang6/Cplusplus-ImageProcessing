#ifndef _COMMITCONTEXT_H_
#define _COMMITCONTEXT_H_

/*
 * file:	commitContext.h
 * purpose:	CommitContext is a place holder for the commitment information,
 *          one per storage commitment target. CommitContextPool manages the
 *          group of storage commitment targets.
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */

#include "control/connectcamera.h"
#include "commitStrategy.h"

class CommitContext
{
public:
	CommitConfig              _commitConfig;
	CommitStrategy*           _commitStrategy;
	list<pthread_t>           _tid;
	DICOMStoragePkg::ResultByStorageTargetList _resultByStorageTargets;

public:

	CommitContext(int applicationID, const CommitConfig& commitConfig, 
				const DICOMStoragePkg::ResultByStorageTargetList& resultByStorageTargets,
				CommitStrategy* commitStrategy);
	~CommitContext();
	CommitContext(const CommitContext& obj);
	CommitContext& operator=(const CommitContext& obj);

	list<pthread_t> execute();

	// return true if retnValue is usable
	bool getResult(DICOMStoragePkg::ResultByCommitTarget& retnValue);
	void cleanCommitStrategy();

private:
	vector<COMMIT_ARGS>       _commitArgs;

	COMMIT_OPTIONS            buildOptions();
};

class CommitContextPool
{
	list<CommitContext>    _pool;
	list<list<pthread_t> > _tid;
	ConnectCamera*         _cameraConnection;

	void cleanCommitStrategies();

public:

	CommitContextPool();
	~CommitContextPool();
	CommitContextPool(const CommitContextPool& obj);
	CommitContextPool& operator=(const CommitContextPool& obj);

	void operator+=(CommitContext& obj);
	void clear();

	list<list<pthread_t> > execute();
	void setCamera(ConnectCamera*& cameraConnection);
	static void* getResult(void* thisClass);
};

#endif
