#ifndef _STORAGESTRATEGY_H_
#define _STORAGESTRATEGY_H_

/*
 * file:	storageStrategy.h
 * purpose:	provides storage algorithm
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */

#include <string>
#include "storage.h"
#include "cstoreutils.h"

using namespace std;

class StorageStrategy
{
protected:
	int    _applicationID;

public:
	StorageStrategy(int applicationID);
	virtual ~StorageStrategy();

	StorageStrategy(const StorageStrategy& obj);
	StorageStrategy& operator=(const StorageStrategy& obj);

	virtual pthread_t storageAlgorithm(const DICOMStoragePkg::DICOMTarget& storeTarget,
									StorageData& storageData);
};

#endif
