/*
 * file:	mppsimpl.cc
 * purpose:	Implementation of the CORBA abstract class
 *
 * revision history:
 *   Jiantao Huang		    initial version
 *
 */

#include <acquire/acqexception.h>
#include <control/orbutils.h>
#include <control/orbnaming.h>
#include "mppsimpl.h"


MppsImpl* MppsImpl::_instance = NULL;
bool MppsImpl::_isShuttingDown = false;

// Ask the ORB to stop nicely
void MppsImpl::
shutdown()
{
	if ( _isShuttingDown )
		return;
	_isShuttingDown = true;

	if ( _instance )
		_instance->shutdownPortal();
}

// Invoke ORB run, does not return until someone has called shutdown.
void MppsImpl::
run( const char *mergeIniFile, const char *mppsName, const char *facilityName, DictionaryPkg::VersionInfo * versionInfo )
{
	/*
	 * static methods remembers what name we wanted
	 * when a connection is finally attempted.
	 */
	ConnectFacility::useName( facilityName);

	const char *exceptionMessage = "";
	try
	{
		exceptionMessage = "cannot create mppsImpl";
		_instance = new MppsImpl( mergeIniFile, mppsName, versionInfo );

		// Toss the main mpps object in a separate POA
		exceptionMessage = "cannot add mpps to poa";
		PortableServer::POA_var poa = ORBUtils::createPersistentPOA( mppsName );
		PortableServer::ObjectId_var mppsId = PortableServer::string_to_ObjectId(mppsName);
		poa->activate_object_with_id(mppsId.in(), _instance);

		// Register main mpps object with naming service
		exceptionMessage = "cannot register mpps with naming service";
		DICOMPkg::PerformedProcedureStep_var pps_var = _instance->_this();
		ORBUtils::namingServiceRebind( mppsName, pps_var.in());

		// Start up poa manager
		exceptionMessage = "cannot start poa manager";
		PortableServer::POAManager_var poa_manager = ORBUtils::getPOAManager();
		poa_manager->activate ();
	}
	catch( ... )
	{
		::Message( MALARM, MLoverall | toEndUser | toService | toDeveloper, "Cannot make mppsscu available to network: %s", exceptionMessage );
		delete _instance;
		_instance = NULL;
		throw;
	}

	::Message( MNOTE, MLoverall, "Mpps started." );

	try
	{
		exceptionMessage = "no orb";
		CORBA::ORB_var orb = ORBUtils::getORB();

		orb->run();
	}
	catch( const CORBA::Exception& e)
	{
		::Message( MALARM, MLoverall | toEndUser | toService | toDeveloper, "Mppsscu ORB terminated prematurely");
		sleep(1);
		throw;
	}
	::Message( MSTATUS, MLoverall, "Mppsscu is stopping, ORB processing complete." );

	try
	{
		delete _instance;
	}
	catch (...)
	{
		::Message( MNOTE, MLoverall, "MppsImpl::run() ignoring exception while deleting _instance");
	}
	_instance = NULL;

	try
	{
		ORBUtils::destroy();
	}
	catch( const CORBA::Exception& e)
	{
		::Message( MALARM, MLoverall | toEndUser | toService | toDeveloper, "MppsImpl::run() caught exception shutting down orb");
		sleep(1);
		throw;
	}

	_isShuttingDown = false;

	::Message( MSTATUS, MLoverall, "MPPS SCU is stopping, ORB destroyed." );
}

MppsImpl::MppsImpl( const char *mergeIniFile, const char *mppsName, DictionaryPkg::VersionInfo * versionInfo ) :
		PortalImpl(mppsName)
{
	_mppsName = CORBA::string_dup(mppsName);
    _versionInfo = new DictionaryPkg::VersionInfo(*versionInfo);

    try {
         _myMppsManager = new MppsManager( mergeIniFile);
    }
    catch( ...) {
	::Message( MALARM, MLoverall | toEndUser, "Cannot create the mpps object.");
        throw;
    }
}

void MppsImpl::
deactivate()
{
	// deactivate main MPPS SCU that is in a different POA
	PortableServer::POA_var poa = ORBUtils::getPOA(_mppsName);
	DICOMPkg::PerformedProcedureStep_ptr ppsp = _instance->_this();
	PortableServer::ObjectId_var obj =   poa->reference_to_id( ppsp );
	poa->deactivate_object( obj.in() );
	CORBA::release( ppsp );
}

MppsImpl::~MppsImpl() throw()
{
	Message( MSTATUS, "MPPS SCU stopping...");

	delete _myMppsManager;
}

StationPkg::Session_ptr MppsImpl::
startSession( StationPkg::Station_ptr sptr, DictionaryPkg::UserType userType)
throw( DictionaryPkg::NucMedException)
{
	return NULL;
}

char* MppsImpl::inProgress(const PatientPkg::ScheduledVisit& currentVisit)
		throw (DictionaryPkg::NucMedException)
{
	try {
		return CORBA::string_dup(_myMppsManager->startMPPSImageAcquisition(currentVisit));
	}
	catch (...) {
		throw (DictionaryPkg::NucMedException(DictionaryPkg::NUCMED_NETWORK));
	}
}

char* MppsImpl::completed(const PatientPkg::ScheduledVisit& currentVisit)
		throw (DictionaryPkg::NucMedException)
{
	try {
		return CORBA::string_dup(_myMppsManager->completeMPPS(currentVisit));
	}
	catch (...) {
		throw (DictionaryPkg::NucMedException(DictionaryPkg::NUCMED_NETWORK));
	}
}

char* MppsImpl::discontinued(const PatientPkg::ScheduledVisit& currentVisit)
		throw (DictionaryPkg::NucMedException)
{
	try {
		return CORBA::string_dup(_myMppsManager->discontinueMPPS(currentVisit));
	}
	catch (...) {
		throw (DictionaryPkg::NucMedException(DictionaryPkg::NUCMED_NETWORK));
	}
}


void MppsImpl::
remoteDebug( const char *action )
{
	// do something ourselves based on string command in "action"

	// optionally call one from portalimpl (may not always want/need to call this one)
	PortalImpl::remoteDebug(action);
}

MppsImpl &MppsImpl::
operator=( const MppsImpl &)
{
	const char *exceptionMessage = "Don't use MppsImpl operator=";
	Message( MALARM, MLproblem, exceptionMessage );
	throw AcqExceptionNotImplemented( exceptionMessage );
}

MppsImpl::
MppsImpl( const MppsImpl &) :
  PortalImpl( DICOMPkg::PerformedProcedureStep::COMPONENT_LABEL )
{
	const char *exceptionMessage = "Don't use MppsImpl copy constructor";
	Message( MALARM, MLproblem, exceptionMessage );
	throw AcqExceptionNotImplemented( exceptionMessage );
}
