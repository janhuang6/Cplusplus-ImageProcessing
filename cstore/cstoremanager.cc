/*
 * file:	cstoremanager.cc
 * purpose:	Implementation of the CstoreManager class
 * created:	24-Aug-07
 * property of:	Philips Nuclear Medicine
 *
 * revision history:
 *   24-Aug-07	Jiantao Huang		    initial version
 */

#include <unistd.h>
#include <stdlib.h>
#include <thread.h>
#include <ctype.h>
#include <math.h>
#include <syslog.h>

#include "cstoremanager.h"
#include "control/lookupmatchutils.h"
#include "control/stationimpl.h"

static const char rcsName[] = "$Name: Atlantis710R01_branch $";
extern const char *cstoreBuildDate;
extern int	AsyncCommitIncomingPort;
extern char LocalSystemCallingAE[AE_LENGTH+2];

CstoreManager* CstoreManager::_instance = NULL;  /* handle of singleton object */

CstoreManager* CstoreManager::instance(const char *mergeIniFile) throw ( DictionaryPkg::NucMedException)
{   if (!_instance && !mergeIniFile) {
	  ::Message( MWARNING, MLoverall | toService | toDeveloper, "#Software error: Cannot construct CstoreManager without mergeIniFile");
      throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
	}

	if (!_instance)
		_instance = new CstoreManager(mergeIniFile);

	return _instance;
}

CstoreManager::CstoreManager(const char *mergeIniFile)
{
  pthread_attr_t attr;

  if(mergeIniFile==NULL || strlen(mergeIniFile) == 0) {
      ::Message( MNOTE, MLoverall | toService | toDeveloper, 
		  "#Software error: Merge INI file is NULL.");
      throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
  }

  strncpy(_mergeIniFile, mergeIniFile, sizeof(_mergeIniFile));
  _cameraConnection = NULL;

  GetCstoreDefaultParameters();

  if(false == PerformMergeInitialization( _mergeIniFile, &_applicationID, LocalSystemCallingAE ))
  {
	::Message( MWARNING, MLoverall | toService | toDeveloper, "#Software error: Cannot initialize the Merge Toolkit.");
	throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
  }
  ::Message( MNOTE, MLoverall | toService | toDeveloper, "Merge Toolkit is initialized with local AE Title %s",
			 LocalSystemCallingAE );

#ifdef linux
  pthread_t tid;

  _echoArgs.EchoIncomingPort = AsyncCommitIncomingPort;
  _echoArgs.ApplicationID = _applicationID;
#ifdef DEBUG_PRINTF
  _echoArgs.Verbose = true;
#else
  _echoArgs.Verbose = false;
#endif
  pthread_attr_init(&attr); // Initialize with the default value
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if( pthread_create(&tid, &attr, echoSCP, (void*)&_echoArgs) != 0)
  {
	::Message( MALARM, toEndUser | toService | MLoverall, 
			   "Cstore failed to create a thread to listen to C-ECHO-RQ");
  }
#endif
}

CstoreManager::CstoreManager(const CstoreManager&)
{
}

CstoreManager& CstoreManager::operator=(const CstoreManager&)
{
	return *this;
}

CstoreManager::~CstoreManager() throw()
{
	ConnectCamera::releaseInstance();

    /*
    ** The last thing that we do is to release this application from the
    ** library.
    */
    MC_Release_Application( &_applicationID );

    if (MC_Library_Release() != MC_NORMAL_COMPLETION)
		::Message( MALARM, toEndUser | toService | MLoverall, 
					"Error releasing the library.");
}


void
CstoreManager::storeAndCommit(const list<string>& filelist,
							const list<DICOMStoragePkg::StorageTarget>& storagetargetlist,
							const list<DICOMStoragePkg::CommitTarget>& committargetlist)
{	DICOMStoragePkg::ResultByStorageTargetList_var resultByStorageTargets;

	resultByStorageTargets = store(filelist, storagetargetlist);
	// Now do the storage commitment
	commit(resultByStorageTargets.in(), committargetlist);
}


DICOMStoragePkg::ResultByStorageTargetList* 
CstoreManager::store(const list<string>& filelist,
					const list<DICOMStoragePkg::StorageTarget>& storagetargetlist)
{	StorageContextPool storageContextPool;
	StorageData storageData;
	char localAETitle[AE_LENGTH+2];
	int minLen = 0;
	DICOMStoragePkg::ResultByStorageTargetList_var resultByStorageTargets;

	if( !filelist.size() )
	{
		::Message(MWARNING, toEndUser | toService | MLoverall, "There is no valid DICOM file to store for this task. Cstore waits for the next task.");
		throw( DictionaryPkg::NucMedException (DictionaryPkg::NUCMED_SOFTWARE) );
	}

	if( !storagetargetlist.size() )
	{
		::Message(MWARNING, toEndUser | toService | MLoverall, "There is no valid DICOM storage target given for this task. The following file(s) will not be stored:");
		for(list<string>::const_iterator iter=filelist.begin(); iter != filelist.end(); ++iter)
			::Message(MWARNING, toEndUser | toService | MLoverall, "%s", iter->c_str());

		::Message(MWARNING, toEndUser | toService | MLoverall, "Cstore waits for the next task");
		throw( DictionaryPkg::NucMedException (DictionaryPkg::NUCMED_SOFTWARE) );
	}
	::Message(MNOTE, toEndUser | toService | MLoverall, "Number of storage targets = %d", (int)storagetargetlist.size());
#ifdef DEBUG_PRINTF
	printf("Number of storage targets = %d\n", (int)storagetargetlist.size());
#endif

	strcpy(localAETitle, storagetargetlist.begin()->exportSystem.localAETitle.in());
	minLen = strlen(localAETitle)<strlen(LocalSystemCallingAE)?strlen(localAETitle):strlen(LocalSystemCallingAE); 
	if (strncmp(localAETitle, LocalSystemCallingAE, minLen))
		::Message( MWARNING, MLoverall | toService | toDeveloper, "LocalSystemCallingAE is not the same in systems.xml(ImageReceiver: %s) as in cstoredefaults.txt: %s",
					localAETitle, LocalSystemCallingAE);

	StorageStrategy storeStrategy(_applicationID);
	storageData.createLinkedList(filelist);

	for(list<DICOMStoragePkg::StorageTarget>::const_iterator iter=storagetargetlist.begin();
		iter != storagetargetlist.end(); ++iter)
	{
		StorageContext sc(*iter, storageData, storeStrategy);
		storageContextPool += sc;
	}

	storageContextPool.execute(); // execute will create one thread per storage target
	resultByStorageTargets = storageContextPool.getResult(); // getResult will wait until threads finish

	openlog( "", LOG_NDELAY | LOG_NOWAIT, LOG_LOCAL7);
	// Check the storage results here. If not successful, throw it all the way to
	// the controller or image handling server so they know that storage failed.
	// If it is successful, send a message to Audit Logs.
	for(int i=0; i<(int)resultByStorageTargets->length(); i++)
		for(int j=0; j<(int)(resultByStorageTargets[i].resultByFiles.length()); j++)
			if(resultByStorageTargets[i].resultByFiles[j].storageOutcome != DICOMStoragePkg::STORAGE_SUCCEESS)
			{	::Message( MWARNING, MLoverall | toService | toDeveloper, "Storage is not successful. Possibly network problem");
				throw( DictionaryPkg::NucMedException (DictionaryPkg::NUCMED_NETWORK) );
			}
			else
			{	// Log to the Audit
				syslog(LOG_NOTICE, "Successfully exported image \"%s\" to storage target \"%s\"", resultByStorageTargets[i].resultByFiles[j].imgFile.in(), resultByStorageTargets[i].storageHostName.in());
			}
	closelog();

	return resultByStorageTargets._retn();
}


void
CstoreManager::commit(const DICOMStoragePkg::ResultByStorageTargetList& resultByStorageTargets,
					const list<DICOMStoragePkg::CommitTarget>& committargetlist)
{	CommitContextPool* pCommitContextPool;
	CommitStrategy*    commitStrategy;
	CommitConfig       commitConfig;
	pthread_t          tid;
	pthread_attr_t     attr;

	if( !committargetlist.size() )
	{
		::Message(MWARNING, toEndUser | toService | MLoverall, "There is no commit target given for this task. Cstore waits for the next task");
		throw( DictionaryPkg::NucMedException (DictionaryPkg::NUCMED_SOFTWARE) );
	}
	::Message(MNOTE, toEndUser | toService | MLoverall, "Number of commit targets = %d", (int)committargetlist.size());
#ifdef DEBUG_PRINTF
	printf("Number of commit targets = %d\n", (int)committargetlist.size());
#endif

	pCommitContextPool = new CommitContextPool;
	for(list<DICOMStoragePkg::CommitTarget>::const_iterator iter=committargetlist.begin();
		iter != committargetlist.end(); ++iter)
	{
		switch (iter->type)
		{
			case DICOMStoragePkg::NO_COMMIT: // Storage without commitment
			{
				commitStrategy = new NoCommitStrategy();
				break;
			}
			case DICOMStoragePkg::SYNCH_COMMIT:
			{
				commitStrategy = new SynchronousCommitStrategy();
				break;
			}
			case DICOMStoragePkg::ASYNCH_COMMIT:
			{
				commitStrategy = new AsynchronousCommitStrategy();
				break;
			}
			case DICOMStoragePkg::EITHER_COMMIT:
			{
				commitStrategy = new EitherCommitStrategy();
				break;
			}
			default:
				continue;
		}

		commitConfig.reportingSystem = iter->reportingSystem;
		commitConfig.roleReversalWaitTime = iter->roleReversalWaitTime;

		// each CommitContext will make a new copy of resultByStorageTargets for itself
		CommitContext cmtcxt(_applicationID, commitConfig, resultByStorageTargets, commitStrategy);
		(*pCommitContextPool) += cmtcxt;
	}

	pCommitContextPool->execute(); // multiple threads will be created
	pCommitContextPool->setCamera(_cameraConnection);

	pthread_attr_init(&attr); // Initialize with the default value
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	// pCommitContextPool will be deleted when the following thread finishes
	if( pthread_create(&tid, &attr, CommitContextPool::getResult, (void *)pCommitContextPool) != 0)
        Message( MWARNING, "#Software error: cannot create a thread to send back commit results.");
}
