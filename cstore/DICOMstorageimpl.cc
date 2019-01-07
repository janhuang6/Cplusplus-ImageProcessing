/*
 * file:	DICOMstorageimpl.cc
 * purpose:	Implementation of the CORBA abstract class
 *
 * revision history:
 *   Jiantao Huang		    initial version
 *
 * inspection history:
 */

#include <stdexcept>
#include <control/orbutils.h>
#include <control/orbnaming.h>
#include "DICOMstorageimpl.h"
#include "cstoremanager.h"

DICOMStorageImpl*  DICOMStorageImpl::_instance = NULL;
bool DICOMStorageImpl::_isShuttingDown = false;

// Ask the ORB to stop nicely
void DICOMStorageImpl::
shutdown()
{
	if ( _isShuttingDown )
		return;
	_isShuttingDown = true;

	if ( _instance )
		_instance->shutdownPortal();
}

// Invoke ORB run, does not return until someone has called shutdown.
void DICOMStorageImpl::
run( const char *mergeIniFile, const char *cstoreName, DictionaryPkg::VersionInfo * versionInfo )
{
	const char *exceptionMessage = "";
	try
	{
		exceptionMessage = "no orb";
		CORBA::ORB_var orb = ORBUtils::getORB();

		exceptionMessage = "cannot create DICOMStorageImpl";
		_instance = NULL;
		if (( _instance = new DICOMStorageImpl( mergeIniFile, cstoreName, versionInfo )) == NULL)
		{
			::Message( MALARM, MLoverall | toEndUser | toService | toDeveloper, 
					   "Cannot create new DICOMStorageImpl.");
			throw;
		}

		// Toss the main cstore object in a separate POA
		exceptionMessage = "cannot add cstore to poa";
		PortableServer::POA_var poa = ORBUtils::createPersistentPOA( cstoreName );
		PortableServer::ObjectId_var cstoreId = PortableServer::string_to_ObjectId(cstoreName);
		poa->activate_object_with_id(cstoreId.in(), _instance);

		// Register main cstore object with naming service
		exceptionMessage = "cannot register cstore with naming service";
		DICOMStoragePkg::DICOMStorage_var DCMstore_var = _instance->_this();
		ORBUtils::namingServiceRebind( cstoreName, DCMstore_var.in());

		// Start up poa manager
		exceptionMessage = "cannot start poa manager";
		PortableServer::POAManager_var poa_manager = ORBUtils::getPOAManager();
		poa_manager->activate ();
	}
	catch( ... )
	{
		::Message( MALARM, MLoverall | toEndUser | toService | toDeveloper, "Cannot make Cstore Server available to network: %s", exceptionMessage );
		if(_instance) delete _instance;
		_instance = NULL;
		throw;
	}

	::Message( MNOTE, MLoverall, "Cstore started." );

	try
	{
		CORBA::ORB_var orb = ORBUtils::getORB();
		orb->run();
	}
	catch( const CORBA::Exception& e)
	{
		::Message( MALARM, MLoverall | toEndUser | toService | toDeveloper, "cstore ORB terminated prematurely");
		sleep(1);
		throw;
	}
	::Message( MSTATUS, MLoverall, "cstore is stopping, ORB processing complete." );

	try
	{
		delete _instance;
	}
	catch (...)
	{
		::Message( MNOTE, MLoverall, "DICOMStorageImpl::run() ignoring exception while deleting _instance");
	}
	_instance = NULL;

	try
	{
		ORBUtils::destroy();
	}
	catch( const CORBA::Exception& e)
	{
		::Message( MALARM, MLoverall | toEndUser | toService | toDeveloper, "DICOMStorageImpl::run() caught exception shutting down orb");
		sleep(1);
		throw;
	}

	_isShuttingDown = false;

	::Message( MSTATUS, MLoverall, "Cstore is stopping, ORB destroyed." );
}

DICOMStorageImpl::DICOMStorageImpl( const char *mergeIniFile, const char *cstoreName, DictionaryPkg::VersionInfo * versionInfo )
		throw( DictionaryPkg::NucMedException ):
		PortalImpl(cstoreName)
{
	_cstoreName = CORBA::string_dup(cstoreName);
    _versionInfo = new DictionaryPkg::VersionInfo(*versionInfo);

    try {
		_cstoreManager = CstoreManager::instance( mergeIniFile);
    }
    catch( ...) {
        ::Message( MALARM, MLoverall | toEndUser, "Cannot create the CstoreManager object.");
        throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
    }
}

void DICOMStorageImpl::
deactivate()
{
	// deactivate main DICOMStorage that is in a different POA
	PortableServer::POA_var poa = ORBUtils::getPOA(_cstoreName);
	DICOMStoragePkg::DICOMStorage_ptr cstorep = _instance->_this();
	PortableServer::ObjectId_var obj =   poa->reference_to_id( cstorep );
	poa->deactivate_object( obj.in() );
	CORBA::release( cstorep );
}

DICOMStorageImpl::~DICOMStorageImpl() throw()
{
	::Message(MSTATUS, toEndUser | toService | MLoverall, "Cstore server stopping...");

	delete _cstoreManager;
}

StationPkg::Session_ptr DICOMStorageImpl::
startSession( StationPkg::Station_ptr sptr, DictionaryPkg::UserType userType)
throw( DictionaryPkg::NucMedException)
{
	return NULL;
}

void
DICOMStorageImpl::storeAndCommit(const DICOMStoragePkg::FileNameList& fileNames,
								const DICOMStoragePkg::StorageTargetList& storageTargets,
								const DICOMStoragePkg::CommitTargetList& commitTargets)
{	list<string> filelist;
	list<DICOMStoragePkg::StorageTarget> storagetargetlist;
	list<DICOMStoragePkg::CommitTarget> committargetlist;
	int i;

	for(i=0; i<(int)fileNames.length(); i++)
	{	if (UNKNOWN_FORMAT != CheckFileFormat(fileNames[i].in()))
			filelist.push_back(fileNames[i].in());
		else
			::Message(MWARNING, toEndUser | toService | MLoverall, "File [%s] doesn't exist or not in DICOM format. Cstore will skip it.", fileNames[i].in());
	}

	for(i=0; i<(int)storageTargets.length(); i++)
		storagetargetlist.push_back(storageTargets[i]);

	for(i=0; i<(int)commitTargets.length(); i++)
		committargetlist.push_back(commitTargets[i]);

	_cstoreManager->storeAndCommit(filelist, storagetargetlist, committargetlist);
}

DICOMStoragePkg::ResultByStorageTargetList*
DICOMStorageImpl::store(const DICOMStoragePkg::FileNameList& fileNames,
						const DICOMStoragePkg::StorageTargetList& storageTargets)
{	
	list<string> filelist;
	list<DICOMStoragePkg::StorageTarget> storagetargetlist;
	int i;

	for(i=0; i<(int)fileNames.length(); i++)
	{	if (UNKNOWN_FORMAT != CheckFileFormat(fileNames[i].in()))
			filelist.push_back(fileNames[i].in());
		else
			::Message(MWARNING, toEndUser | toService | MLoverall, "Skip file [%s] because it either doesn't exist or is not in DICOM format.", fileNames[i].in());
	}

	for(i=0; i<(int)storageTargets.length(); i++)
		storagetargetlist.push_back(storageTargets[i]);

	return _cstoreManager->store(filelist, storagetargetlist);
}


void
DICOMStorageImpl::commit(const DICOMStoragePkg::ResultByStorageTargetList& UIDList,
						 const DICOMStoragePkg::CommitTargetList& commitTargets)
{	list<DICOMStoragePkg::CommitTarget>            committargetlist;
	int                                            i, j, cnt = 0;
	DICOMStoragePkg::ResultByStorageTargetList_var tmpResult;
	DICOMStoragePkg::ResultByStorageTargetList_var resultByStorageTargets;

	// Convert from IDL sequence to STL list
	for(i=0; i<(int)commitTargets.length(); i++)
		committargetlist.push_back(commitTargets[i]);

	// The following code removes the UIDList element which doesn't need storage commit
	tmpResult = new DICOMStoragePkg::ResultByStorageTargetList;
	tmpResult->length(UIDList.length());

    for(i=0; i < (int)UIDList.length(); i++)
		if (UIDList[i].storageCommitRequired == true)
			cnt++;

	resultByStorageTargets = new DICOMStoragePkg::ResultByStorageTargetList;
	resultByStorageTargets->length(cnt);

	j=0;
    for(i=0; i < (int)UIDList.length(); i++)
	{	if (UIDList[i].storageCommitRequired == true)
			resultByStorageTargets[j++]=UIDList[i];
#ifdef DEBUG_PRINTF
		else
		{	// Skip this UIDList[i] because storageCommit is not required
			::Message(MNOTE, toEndUser | toService | MLoverall, 
					"Storage commit is not required for this UIDList. The intermediate result is skipped");
		}
#endif
	}
	
	return _cstoreManager->commit(resultByStorageTargets.in(), committargetlist);
}


void DICOMStorageImpl::
remoteDebug( const char *action )
{
	// do something ourselves based on string command in "action"

	// optionally call one from portalimpl (may not always want/need to call this one)
	PortalImpl::remoteDebug(action);
}

DICOMStorageImpl &DICOMStorageImpl::
operator=( const DICOMStorageImpl &)
{
	const char *exceptionMessage = "Don't use DICOMStorageImpl operator=";
	Message( MALARM, MLproblem, exceptionMessage );
	throw std::runtime_error( exceptionMessage );
}

DICOMStorageImpl::
DICOMStorageImpl( const DICOMStorageImpl &) :
  PortalImpl( "DICOMStorage")
{
	const char *exceptionMessage = "Don't use DICOMStorageImpl copy constructor";
	Message( MALARM, MLproblem, exceptionMessage );
	throw std::runtime_error( exceptionMessage );
}
