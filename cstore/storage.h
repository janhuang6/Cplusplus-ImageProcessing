#ifndef _STORAGE_H_
#define _STORAGE_H_

/*
 * file:	storage.h
 * purpose:	Storage method class that stores a list of DICOM files
 *          into one Target
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */

#include "cstoreutils.h"

class Storage
{
	int             _applicationID; /* One per app, after registered    */
	DICOMStoragePkg::DICOMTarget _remoteTarget;
	STORAGE_OPTIONS _options; /* configurations for the storage */

	bool populateOptions();

public:
	Storage(int applicationID);
	~Storage() throw ();
	Storage(const Storage& obj);
	Storage& operator=(const Storage& obj);

	pthread_t storage(const DICOMStoragePkg::DICOMTarget& storagetarget,
					StorageData& storageData);
};

#endif
