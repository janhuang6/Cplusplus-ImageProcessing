/*
 * file:	storageStrategy.cc
 * purpose:	Implementation of the storage algorithm
 * created:	08-Nov-07
 * property of:	Philips Nuclear Medicine
 *
 * revision history:
 *   08-Nov-07	Jiantao Huang		    initial version
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
