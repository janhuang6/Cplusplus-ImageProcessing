#ifndef _CSTOREMANAGER_H_
#define _CSTOREMANAGER_H_

/*
 * file:	cstoremanager.h
 * purpose:	CstoreManager sigleton class
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */


#include "control/traitutils.h"
#include "control/connectfacility.h"
#include "control/connectfacilitysession.h"
#include "cstoreutils.h"

#include "acquire/server.h"
#include "acquire/threadmutex.h"
#include "storageContext.h"
#include "commitContext.h"

static const char componentName[] = "cstore";
class CommitReport;

class CstoreManager : public MesgProducer {
	int                   _applicationID;/* One per app, after registered    */
	static CstoreManager* _instance; /* Singleton instance */
	char                  _mergeIniFile[256];
    ConnectCamera*        _cameraConnection;
#ifdef linux
	struct ECHO_ARGS	  _echoArgs;
#endif

	CstoreManager( const char *mergeIniFile);

	// Disallow copying or assignment.
	CstoreManager(const CstoreManager&);
	CstoreManager& operator=(const CstoreManager&);

public:
	static CstoreManager* instance(const char *mergeIniFile=NULL)
			throw ( DictionaryPkg::NucMedException);
	virtual ~CstoreManager() throw();

	// Do Store and Commit in two steps, or Store without Commit
	DICOMStoragePkg::ResultByStorageTargetList* store(const list<string>& filelist,
									const list<DICOMStoragePkg::StorageTarget>& storagetargetlist);
	void commit(const DICOMStoragePkg::ResultByStorageTargetList& resultByStorageTargets,
				const list<DICOMStoragePkg::CommitTarget>& committargetlist);

	// Do Store and Commit in one step
    void storeAndCommit(const list<string>& filelist,
						const list<DICOMStoragePkg::StorageTarget>& storagetargetlist,
						const list<DICOMStoragePkg::CommitTarget>& committargetlist);
};

#endif
