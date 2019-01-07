/*
 * file:	commitContext.cc
 * purpose:	Implementation of the CommitContext and CommitContextPool
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */

#include "commitContext.h"

static const char *cameraName = "camera";
ThreadMutex g_lock_commitWait;  /* Serialize access to the critical section.*/

/*
 * CommitContext class
 */

CommitContext::CommitContext(int applicationID, const CommitConfig& commitConfig, 
							 const DICOMStoragePkg::ResultByStorageTargetList& resultByStorageTargets,
							 CommitStrategy* commitStrategy)
				:_commitConfig (commitConfig),
				 _commitStrategy (commitStrategy),
				 _tid (0),
				 _resultByStorageTargets (resultByStorageTargets)
{
	unsigned int i;
	COMMIT_ARGS  commitArgs;

	commitArgs.options = buildOptions();
	commitArgs.appID   = applicationID;

    /*
     * Now, build _commitArgs to point to local _resultByStorageTargets
     */
	for(i=0; i<_resultByStorageTargets.length(); i++)
	{
		commitArgs.result = const_cast<DICOMStoragePkg::ResultByStorageTarget*>(&(_resultByStorageTargets[i]));
		_commitArgs.push_back(commitArgs);
	}
}

CommitContext::~CommitContext()
{	// We do not call cleanCommitStrategy() here, which is a running thread.
	// So we return immediately. And leave ~CommitContextPool()
	// to clean all commitStrategies nicely.
	// There should be no memory leak.
}

CommitContext::CommitContext(const CommitContext& obj)
				:_commitConfig (obj._commitConfig),
				 _commitStrategy (obj._commitStrategy),
				 _tid (obj._tid),
				 _resultByStorageTargets (obj._resultByStorageTargets),
				 _commitArgs (obj._commitArgs)
{
    /*
     * Now, redirect _commitArgs to point to local _resultByStorageTargets
     */
	for(unsigned int i=0; i<_resultByStorageTargets.length(); i++)
		_commitArgs[i].result = &(_resultByStorageTargets[i]);
}

CommitContext& CommitContext::operator=(const CommitContext& obj)
{
	_commitConfig = obj._commitConfig;
	_commitStrategy = obj._commitStrategy;
	_tid = obj._tid;
	_resultByStorageTargets = obj._resultByStorageTargets;
	_commitArgs = obj._commitArgs;

    /*
     * Now, redirect _commitArgs to point to local _resultByStorageTargets
     */
	for(unsigned int i=0; i<_resultByStorageTargets.length(); i++)
		_commitArgs[i].result = &(_resultByStorageTargets[i]);

	return *this;
}

list<pthread_t> CommitContext::execute()
{
	_commitStrategy->commitAlgorithm(_commitArgs, _tid);
	return _tid;
}

bool CommitContext::getResult(DICOMStoragePkg::ResultByCommitTarget& retnValue)
{
	DICOMStoragePkg::CommitType ct = _commitStrategy->commitType();
	int                         status;

	if( ct == DICOMStoragePkg::NO_COMMIT )
		return false;

    for(list<pthread_t>::const_iterator iter=_tid.begin(); iter != _tid.end(); ++iter)
	{
		if (*iter == (pthread_t)(-1))
		{
			::Message(MWARNING, toEndUser | toService | MLoverall, 
				"CommitContext::getResult() should not be called before the thread is created.");
			continue;
		}

		// Wait for the thread to finish.
		pthread_join(*iter, (void **)&status);

		if ( status == THREAD_EXCEPTION )
		{
			::Message(MWARNING, toEndUser | toService | MLoverall, 
				"CommitContext::getResult() catches thread exception.");
			throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_NETWORK ) );
			return false;
		}
	}

    /*
     * Now, build and return the table
     */
	retnValue.commitHostName = CORBA::string_dup(_commitConfig.reportingSystem.hostName);
	retnValue.resultByStorageTargets = _resultByStorageTargets;

    return true;
}

void CommitContext::cleanCommitStrategy()
{	list<pthread_t>::iterator iter;

    for(iter=_tid.begin(); iter != _tid.end(); ++iter)
	{
		if (*iter != (pthread_t)(-1))
		{
			// Wait for the thread to finish.
			pthread_join(*iter, NULL);
		}
	}

	delete _commitStrategy;
}

/*
 * CommitContextPool class
 */

void CommitContextPool::cleanCommitStrategies()
{	list<CommitContext>::iterator iter;

    for(iter=_pool.begin(); iter != _pool.end(); ++iter)
	{	
		iter->cleanCommitStrategy();
	}
}

CommitContextPool::CommitContextPool()
{
	_cameraConnection = NULL;
}

CommitContextPool::~CommitContextPool()
{
	cleanCommitStrategies();
	// _cameraConnection is used by all instances of CommitContextPool
}

CommitContextPool::CommitContextPool(const CommitContextPool& obj)
					: _pool (obj._pool), _tid (obj._tid),
					  _cameraConnection (obj._cameraConnection)
{
}

CommitContextPool& CommitContextPool::operator=(const CommitContextPool& obj)
{
	_pool = obj._pool;
	_tid = obj._tid;
	_cameraConnection = obj._cameraConnection;

	return *this;
}

void CommitContextPool::operator+=(CommitContext& commitContext)
{	
	_pool.push_back(commitContext); // push_back will copy the content of commitContext
}

void CommitContextPool::clear()
{
	_pool.clear();
	_tid.clear();
	_cameraConnection = NULL; // local copy cleared
}

list<list<pthread_t> > CommitContextPool::execute()
{	list<CommitContext>::iterator iter;

	_tid.clear();
    for(iter=_pool.begin(); iter != _pool.end(); ++iter)
	{	// Each execute returns a list of thread IDs
		_tid.push_back(iter->execute());
	}

	return _tid;
}

void CommitContextPool::setCamera(ConnectCamera*& cameraConnection)
{
	if (cameraConnection == NULL)
	{
		//... perform necessary instance initializations 
		int iSleepCount=0;;

		try {
			::Message(MNOTE, toEndUser | toService | MLoverall, 
					"Cstore is checking the connection to the Camera");
			ConnectCamera::useName( cameraName );
			//ConnectCamera::setUserType(DictionaryPkg::USER_ADMIN);
			cameraConnection = ConnectCamera::instance();	
		} catch (... ) {
			::Message(MWARNING, toEndUser | toService | MLoverall, 
					"Cstore failed to connect to Camera, and cannot send back the storage commitment results");
			cameraConnection = NULL;
		}

		while (cameraConnection != NULL && 0==cameraConnection->isReady()){ 
			if (iSleepCount == 0)
				fprintf(stderr,"Waiting for connection --");
			sleep(2);
			iSleepCount +=2;
			fprintf(stderr,"\b\b%2d",iSleepCount);
			if (iSleepCount > 30){
				fprintf(stderr,"\n");
				::Message(MWARNING, toEndUser | toService | MLoverall, 
						"Cstore connection to Camera timed out");
				cameraConnection = NULL;
			}
		}
		fprintf(stderr,"\n");
	}

	// Finally, keep a local copy
	_cameraConnection = cameraConnection;
}

void* CommitContextPool::getResult(void* thisClass)
{	MutexGuard guard (g_lock_commitWait);

	CommitContextPool *pCommitContextPool;
	list<CommitContext>::iterator iter;
	DICOMStoragePkg::CommitReport_var commitReport;
	char hasResult = 0;
	int i = 0;

	pCommitContextPool = (CommitContextPool*)thisClass;
	commitReport = new DICOMStoragePkg::CommitReport;
	commitReport->length(pCommitContextPool->_pool.size());

	try
	{
		for(iter=pCommitContextPool->_pool.begin(); iter != pCommitContextPool->_pool.end(); ++iter, ++i)
		{	DICOMStoragePkg::ResultByCommitTarget resultByCommitTarget;

			if (iter->getResult(resultByCommitTarget))
			{
				commitReport[i]=resultByCommitTarget;
				hasResult = 1; // There will be no result if NoCommit on
			}
		}
	}
	catch(...)
	{
		::Message(MWARNING, toEndUser | toService | MLoverall,
				"CommitContextPool::getResult() caught exception\n");
		delete pCommitContextPool;
		guard.release();
		pthread_exit( NULL );
	}

	// Call the controller or imageHandlingServer back to return the commit results;
	// jhuang 5/15/08

	try
	{
		if ( hasResult )
		{
			if ( pCommitContextPool->_cameraConnection != NULL )
				pCommitContextPool->_cameraConnection->reportCommitStatus(commitReport.in());
			else
				::Message(MWARNING, toEndUser | toService | MLoverall, 
						"Cstore is unable to report Commit Status due to no connection");
		}
		else
			::Message(MNOTE, toEndUser | toService | MLoverall, 
					"Skip storage commitment because NoCommit is on for all commit targets");
	}
	catch( ...)
	{
		::Message(MWARNING, toEndUser | toService | MLoverall, 
				"Exception was thrown when reporting Commit Status");
	}

	delete pCommitContextPool;
	guard.release();
	return NULL;
}

COMMIT_OPTIONS CommitContext::buildOptions()
{
	COMMIT_OPTIONS options;

	/*
     * Set default values
     */
    strcpy(options.RemoteAE, _commitConfig.reportingSystem.remoteAETitle.in());
    strcpy(options.RemoteHostname, _commitConfig.reportingSystem.hostName.in());
    options.RemotePort = _commitConfig.reportingSystem.portNumber;
    options.RoleReversalWaitTime = _commitConfig.roleReversalWaitTime;
    ::Message(MNOTE, toEndUser | toService | MLoverall, 
            "CommitTarget remoteAETitle=%s", _commitConfig.reportingSystem.remoteAETitle.in());
    ::Message(MNOTE, toEndUser | toService | MLoverall, 
            "CommitTarget localAETitle=%s", _commitConfig.reportingSystem.localAETitle.in());
    ::Message(MNOTE, toEndUser | toService | MLoverall, 
            "CommitTarget HostName=%s", _commitConfig.reportingSystem.hostName.in());
    ::Message(MNOTE, toEndUser | toService | MLoverall, 
            "CommitTarget PortNumber=%ld", (long)_commitConfig.reportingSystem.portNumber);
    ::Message(MNOTE, toEndUser | toService | MLoverall, 
            "CommitConfig RoleReversalWaitTime=%ld", (long)_commitConfig.roleReversalWaitTime);
#ifdef DEBUG_PRINTF
	printf("CommitTarget remoteAETitle=%s\n", _commitConfig.reportingSystem.remoteAETitle.in());
	printf("CommitTarget localAETitle=%s\n", _commitConfig.reportingSystem.localAETitle.in());
	printf("CommitTarget HostName=%s\n", _commitConfig.reportingSystem.hostName.in());
	printf("CommitTarget PortNumber=%ld\n", (long)_commitConfig.reportingSystem.portNumber);
	printf("CommitConfig RoleReversalWaitTime=%ld\n", (long)_commitConfig.roleReversalWaitTime);
	options.Verbose = true;
#else
	options.Verbose = false;
#endif

    ::Message(MNOTE, toEndUser | toService | MLoverall, 
			"Opening connection to remote system for storage commitment:");
    ::Message(MNOTE, toEndUser | toService | MLoverall,     "Remote AE title:            %s", options.RemoteAE);
    if (options.RemoteHostname[0])
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Remote Hostname:            %s", options.RemoteHostname);
    else
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Remote Hostname:            Default in mergecom.app");
        
    if (options.RemotePort != -1)
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Remote Port:                %d", options.RemotePort);
    else
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Remote Port:                Default in mergecom.app");
        
    ::Message(MNOTE, toEndUser | toService | MLoverall,     "TimeBeforeRoleReverse:      %d seconds", options.RoleReversalWaitTime);

    return options;

}/* buildOptions() */
