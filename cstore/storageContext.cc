/*
 * file:	storageContext.cc
 * purpose:	Implementation of the StorageContext and StorageContextPool
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */

#include "storageContext.h"

/*
 * StorageContext class
 */

StorageContext::StorageContext(const DICOMStoragePkg::StorageTarget& storageTarget, 
							   StorageData& storageData, 
							   StorageStrategy& storageStrategy)
				:_storageTarget (storageTarget),
				 _storageData (storageData),
				 _storageStrategy (storageStrategy),
				 _tid ((pthread_t)-1)
{
}

StorageContext::~StorageContext()
{
}

StorageContext::StorageContext(const StorageContext& obj)
				:_storageTarget (obj._storageTarget),
				 _storageData (obj._storageData),
				 _storageStrategy (obj._storageStrategy),
				 _tid (obj._tid)
{
}

StorageContext& StorageContext::operator=(const StorageContext& obj)
{
	_storageTarget = obj._storageTarget;
	_storageData = obj._storageData;
	_storageStrategy = obj._storageStrategy;
	_tid = obj._tid;

	return *this;
}

pthread_t StorageContext::execute()
{	
	_tid = _storageStrategy.storageAlgorithm(_storageTarget.exportSystem, _storageData);
	return _tid;
}

DICOMStoragePkg::ResultByStorageTarget StorageContext::getResult()
{
	InstanceNode*                          node;
	DICOMStoragePkg::ResultByStorageTarget result;
	int                                    i, status;
	DICOMStoragePkg::ResultByFile          resultByFile;

	if (_tid == (pthread_t)(-1))
	{
		::Message(MWARNING, toEndUser | toService | MLoverall, 
			"StorageContext::getResult() should not be called before the thread is created.");
		return result; // return NULL result
	}

	// Wait for the thread to finish.
	pthread_join(_tid, (void **)&status);
	if ( status == THREAD_EXCEPTION )
	{
		::Message(MWARNING, toEndUser | toService | MLoverall, 
				"StorageContext::getResult() catches thread exception.");
		throw (DictionaryPkg::NucMedException(DictionaryPkg::NUCMED_NETWORK));
	}

	result.storageHostName = CORBA::string_dup(_storageTarget.exportSystem.hostName);
	result.storageCommitRequired = _storageTarget.storageCommitRequired;
//	result.transactionUID will be created in N-ACTION of storage commitment later
	result.resultByFiles.length(GetNumNodes(_storageData._instanceList));

	i = 0;
	node = _storageData._instanceList;
	while (node)
	{
		resultByFile.imgFile = CORBA::string_dup(node->fname);
		resultByFile.SOPClassUID = CORBA::string_dup(node->SOPClassUID);
		resultByFile.SOPInstanceUID = CORBA::string_dup(node->SOPInstanceUID);
		resultByFile.storageOutcome = node->storageStatus; // jhuang 2/20/2009 for MLSDB 29607
		resultByFile.commitOutcome = node->commitStatus; // Commit outcome is unknown yet at this point

		result.resultByFiles[i++]=resultByFile;

        node = node->Next;
	}

	return result;
}


/*
 * StorageContextPool class
 */

StorageContextPool::StorageContextPool()
{
}

StorageContextPool::~StorageContextPool()
{
}

StorageContextPool::StorageContextPool(const StorageContextPool& obj)
					: pool (obj.pool),
					tid (obj.tid)
{
}

StorageContextPool& StorageContextPool::operator=(const StorageContextPool& obj)
{
	pool = obj.pool;
	tid = obj.tid;
	return *this;
}

void StorageContextPool::operator+=(StorageContext& storageContext)
{	
	pool.push_back(storageContext); // push_back will copy the content of storageContext
}

void StorageContextPool::clear()
{
	pool.clear();
	tid.clear();
}

void StorageContextPool::execute()
{	list<StorageContext>::iterator iter;

	tid.clear();
    for(iter=pool.begin(); iter != pool.end(); ++iter)
	{	// Each execute returns a thread ID
		tid.push_back(iter->execute());
	}
}

DICOMStoragePkg::ResultByStorageTargetList*
StorageContextPool::getResult()
{	list<StorageContext>::iterator iter;
	int                                             i = 0, j;
	DICOMStoragePkg::ResultByStorageTargetList_var  tmpResult;
	DICOMStoragePkg::ResultByStorageTargetList_var  resultByStorageTargets;

	tmpResult = new DICOMStoragePkg::ResultByStorageTargetList;
	tmpResult->length(pool.size());

    for(iter=pool.begin(); iter != pool.end(); ++iter)
	{
		tmpResult[i]=iter->getResult();
		if (tmpResult[i].resultByFiles.length()!=0)
			i++;
	}

	resultByStorageTargets = new DICOMStoragePkg::ResultByStorageTargetList;
	resultByStorageTargets->length(i);

    for(j=0; j < i; ++j)
		resultByStorageTargets[j]=tmpResult[j];
	
	return resultByStorageTargets._retn();
}
