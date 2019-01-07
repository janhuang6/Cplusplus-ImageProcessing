/*
 * file:	storage.cc
 * purpose:	Implementation of the Storage class
 * created:	17-Oct-07
 * property of:	Philips Nuclear Medicine
 *
 * revision history:
 *   17-Oct-07	Jiantao Huang		    initial version
 */

#include "storage.h"
#include <time.h>

Storage::Storage(int applicationID)
		:_applicationID (applicationID)
{    
}

Storage::~Storage() throw ()
{
}

Storage::Storage(const Storage& obj)
		:_applicationID (obj._applicationID),
		 _remoteTarget (obj._remoteTarget),
		 _options (obj._options)
{
}

Storage& Storage::operator=(const Storage& obj)
{
	_applicationID = obj._applicationID;
	_remoteTarget = obj._remoteTarget;
	_options = obj._options;

	return *this;
}

bool Storage::populateOptions()
{
    /*
     * Set default values
     */
    strcpy(_options.RemoteAE, _remoteTarget.remoteAETitle.in());
    strcpy(_options.RemoteHostname, _remoteTarget.hostName.in());
    _options.RemotePort = _remoteTarget.portNumber;
    strcpy(_options.ServiceList, "Cstore_SCU_Service_List");
	::Message( MNOTE, MLoverall | toService | toDeveloper,
			"StorageTarget remoteAETitle=%s", _remoteTarget.remoteAETitle.in());
	::Message( MNOTE, MLoverall | toService | toDeveloper,
			"StorageTarget HostName=%s", _remoteTarget.hostName.in());
	::Message( MNOTE, MLoverall | toService | toDeveloper,
			"StorageTarget PortNumber=%ld", (long)_remoteTarget.portNumber);
#ifdef DEBUG_PRINTF
	printf("StorageTarget remoteAETitle=%s\n", _remoteTarget.remoteAETitle.in());
	printf("StorageTarget HostName=%s\n", _remoteTarget.hostName.in());
	printf("StorageTarget PortNumber=%ld\n", (long)_remoteTarget.portNumber);
	_options.Verbose = true;
#else
	_options.Verbose = false;
#endif
    _options.HandleEncapsulated = false; // need input?

    return true;
    
}/* populateOptions() */

pthread_t Storage::storage(const DICOMStoragePkg::DICOMTarget& storagetarget,
						StorageData& storageData)
{
	STORE_ARGS *store_args;
	pthread_t tid;

	_remoteTarget = storagetarget;
	populateOptions();

	store_args = new STORE_ARGS; // The thread will delete it before pthread_exit

	store_args->applicationID = _applicationID;
	store_args->options = _options;
	store_args->storageData = &storageData;

	if( pthread_create(&tid, NULL, StoreFiles, (void*)store_args) != 0)
	{
		::Message( MALARM, toEndUser | toService | MLoverall, 
				"#Software error: cannot create a thread for storage to %s.",
				CORBA::string_dup(storagetarget.hostName));
		return (pthread_t)(-1);
	}

	return tid;
}
