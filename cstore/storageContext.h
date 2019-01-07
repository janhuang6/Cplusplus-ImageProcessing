#ifndef _STORAGECONTEXT_H_
#define _STORAGECONTEXT_H_

/*
 * file:	storageContext.h
 * purpose:	StorageContext is a place holder for storage information,
 *          one per storage target. StorageContextPool manages the
 *          group of storage targets.
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */

#include "storageStrategy.h"

class StorageContext
{
	DICOMStoragePkg::StorageTarget _storageTarget;
	StorageData                    _storageData;
	StorageStrategy                _storageStrategy;
	pthread_t                      _tid;

public:

	StorageContext(const DICOMStoragePkg::StorageTarget& storageTarget,
					StorageData& storageData,
					StorageStrategy& storageStrategy);
	~StorageContext();

	StorageContext(const StorageContext& obj);
	StorageContext& operator=(const StorageContext& obj);

	pthread_t execute();
	DICOMStoragePkg::ResultByStorageTarget getResult();
};

class StorageContextPool
{
	list<StorageContext> pool;
	list<pthread_t>      tid;

public:

	StorageContextPool();
	~StorageContextPool();

	StorageContextPool(const StorageContextPool& obj);
	StorageContextPool& operator=(const StorageContextPool& obj);

	void operator+=(StorageContext&);
	void clear();

	void execute();
	DICOMStoragePkg::ResultByStorageTargetList* getResult();
};

#endif
