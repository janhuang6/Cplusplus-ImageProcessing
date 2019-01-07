#ifndef WORKLISTIMPL_H
#define WORKLISTIMPL_H

/*
 * file:	worklistimpl.h
 * purpose:	Implementation of the CORBA abstract class
 *
 * revision history:
 *   Jiantao Huang           initial version
 * inspection history:
 *   C.Carman                new debugging idl functions
 */

#include <control/dicom_s.h>
#include <control/portal.h>
#include "manualworklist.h"
#include "automatedworklist.h"

class WorklistImpl : public PortalImpl, public POA_DICOMPkg::Worklist {
    ManualWorklist* _manualWorklist;
    AutomatedWorklist* _automatedWorklist;

    char                     *  _worklistName;

    static WorklistImpl      *  _instance;
    static bool                 _isShuttingDown;

    WorklistImpl( const char *mergeIniFile, const char *worklistName, DictionaryPkg::VersionInfo * versionInfo );

    WorklistImpl &operator=( const WorklistImpl &) __attribute__ ((deprecated));
    WorklistImpl( const WorklistImpl &) __attribute__ ((deprecated));

public:

    typedef CORBA::Boolean Boolean;
    typedef DictionaryPkg::NucMedException NucMedException;

    /*
     * Methods for use by the main() method
     */

    static void run( const char *mergeIniFile, const char *worklistName, const char *facilityName, DictionaryPkg::VersionInfo * versionInfo );
    static void shutdown();
    void deactivate();

    virtual ~WorklistImpl() throw();

    /*
     * Implement the pure virtual method of PortalImpl
     */
    StationPkg::Session_ptr startSession( StationPkg::Station_ptr sptr, DictionaryPkg::UserType userType) throw( NucMedException);

    /*
     * Implement six pure virtual methods of _sk_Worklist
     */
    virtual CORBA::Boolean setWorklistProtocolMapping(
        const DICOMPkg::ProtocolMapList& mapping
        ) throw (CORBA::SystemException);
    virtual CORBA::Long getWorkListMatch(
        const DictionaryPkg::LookupMatchList& toMatch
        ) throw (CORBA::SystemException);
    DictionaryPkg::VersionInfo* getVersionInfo() throw( DictionaryPkg::NucMedException ) {
	return PortalImpl::getVersionInfo();
    }
    virtual CORBA::Long getTodaysWorkList() throw (CORBA::SystemException);
    virtual Boolean isScheduledVisitStillValid(
        const PatientPkg::ScheduledVisit& toMatch
        ) throw (CORBA::SystemException);
    virtual CORBA::Boolean setAutomatedWorklistQueryDefaults(
        const DictionaryPkg::LookupMatchList& defaults
        ) throw (CORBA::SystemException);
    CORBA::Boolean isPortalAlive() throw( DictionaryPkg::NucMedException) {
        return PortalImpl::isPortalAlive();
    };

    virtual void remoteDebug( const char *action );
};

#endif
