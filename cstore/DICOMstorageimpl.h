#ifndef DICOMSTORAGEIMPL_H
#define DICOMSTORAGEIMPL_H

/*
 * file:	DICOMstorageimpl.h
 * purpose:	Implementation of the CORBA abstract class
 * created:	24-Aug-07
 * property of:	Philips Nuclear Medicine
 *
 * revision history:
 *   24-Aug-07	Jiantao Huang           initial version
 * inspection history:
 */

#include <control/portal.h>
#include "cstoremanager.h"


class DICOMStorageImpl : public PortalImpl, public POA_DICOMStoragePkg::DICOMStorage {
    char                        *  _cstoreName;
    static DICOMStorageImpl     *  _instance;
    static bool                    _isShuttingDown;
	CstoreManager               *  _cstoreManager;

    DICOMStorageImpl( const char *mergeIniFile, const char *cstoreName, DictionaryPkg::VersionInfo * versionInfo )
        throw( DictionaryPkg::NucMedException );

    DICOMStorageImpl &operator=( const DICOMStorageImpl &) __attribute__ ((deprecated));
    DICOMStorageImpl( const DICOMStorageImpl &) __attribute__ ((deprecated));

public:

//    typedef CORBA::Boolean Boolean;
    typedef DictionaryPkg::NucMedException NucMedException;

    /*
     * Methods for use by the main() method
     */

    static void run( const char *mergeIniFile, const char *cstoreName, DictionaryPkg::VersionInfo * versionInfo );
    static void shutdown();
    void deactivate();

    virtual ~DICOMStorageImpl() throw();

    /*
     * Implement the pure virtual methods of PortalImpl
     */
    StationPkg::Session_ptr startSession( StationPkg::Station_ptr sptr, DictionaryPkg::UserType userType) throw( NucMedException);

    CORBA::Boolean isPortalAlive() throw( DictionaryPkg::NucMedException) {
        return PortalImpl::isPortalAlive();
    }

    /*
     * Implement the pure virtual methods of _impl_DICOMStorage
     */
    virtual void storeAndCommit(
						const DICOMStoragePkg::FileNameList& fileNames,
						const DICOMStoragePkg::StorageTargetList& storageTargets,
						const DICOMStoragePkg::CommitTargetList& commitTargets);
    virtual DICOMStoragePkg::ResultByStorageTargetList* store(
						const DICOMStoragePkg::FileNameList& fileNames,
						const DICOMStoragePkg::StorageTargetList& storageTargets);
    virtual void commit(const DICOMStoragePkg::ResultByStorageTargetList& UIDList,
						const DICOMStoragePkg::CommitTargetList& commitTargets);
    virtual DictionaryPkg::VersionInfo* getVersionInfo() throw( DictionaryPkg::NucMedException ) {
		return PortalImpl::getVersionInfo();
    }

    virtual void remoteDebug( const char *action );
};

#endif
