#ifndef MPPSIMPL_H
#define MPPSIMPL_H

/*
 * file:	mppsimpl.h
 * purpose:	Implementation of the CORBA abstract class
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */

#include <control/dicom_s.h>
#include <control/portal.h>
#include "mppsmanager.h"

class MppsImpl : public PortalImpl, public POA_DICOMPkg::PerformedProcedureStep {
    MppsManager*					_myMppsManager;

    char*							_mppsName;

    static MppsImpl*				_instance;
    static bool						_isShuttingDown;

    MppsImpl( const char *mergeIniFile, const char *mppsName, DictionaryPkg::VersionInfo * versionInfo );

	// Disallow copying or assignment.
    MppsImpl &operator=( const MppsImpl &) __attribute__ ((deprecated));
    MppsImpl( const MppsImpl &) __attribute__ ((deprecated));

public:

    typedef CORBA::Boolean Boolean;
    typedef DictionaryPkg::NucMedException NucMedException;

    /*
     * Methods for use by the main() method
     */

    static void run( const char *mergeIniFile, const char *mppsName, const char *facilityName, DictionaryPkg::VersionInfo * versionInfo );
    static void shutdown();
    void deactivate();

	virtual ~MppsImpl() throw();

    /*
     * Implement the pure virtual method of PortalImpl
     */
    StationPkg::Session_ptr startSession( StationPkg::Station_ptr sptr, DictionaryPkg::UserType userType) throw( NucMedException);

    CORBA::Boolean isPortalAlive() throw( DictionaryPkg::NucMedException) {
        return PortalImpl::isPortalAlive();
    };

    virtual DictionaryPkg::VersionInfo* getVersionInfo() throw( DictionaryPkg::NucMedException ) {
		return PortalImpl::getVersionInfo();
    }


    /*
     * Implement the four pure virtual methods of _sk_Mpps
     */
    virtual char* inProgress(const PatientPkg::ScheduledVisit& currentVisit)
		throw (DictionaryPkg::NucMedException);

	virtual char* completed(const PatientPkg::ScheduledVisit& currentVisit)
		throw (DictionaryPkg::NucMedException);

	virtual char* discontinued(const PatientPkg::ScheduledVisit& currentVisit)
		throw (DictionaryPkg::NucMedException);

    virtual void remoteDebug( const char *action );
};

#endif
