/*
 * file:	storageStrategy.cc
 * purpose:	Implementation of the storage algorithm
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */

#include "storageStrategy.h"

StorageStrategy::StorageStrategy(int applicationID)
				:_applicationID(applicationID)
{
}

StorageStrategy::~StorageStrategy()
{
}

StorageStrategy::StorageStrategy(const StorageStrategy& obj)
				:_applicationID(obj._applicationID)
{
}

StorageStrategy& StorageStrategy::operator=(const StorageStrategy& obj)
{
	_applicationID = obj._applicationID;

	return *this;
}

pthread_t StorageStrategy::storageAlgorithm(const DICOMStoragePkg::DICOMTarget& storeTarget,
										StorageData& storageData)
{	Storage storage(_applicationID);
	return storage.storage(storeTarget, storageData);
}
