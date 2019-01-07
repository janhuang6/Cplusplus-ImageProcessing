/*
 * file:	worklistimpl.cc
 * purpose:	Implementation of the CORBA abstract class
 *
 * revision history:
 *   Jiantao Huang		    initial version
 *
 * inspection history:
 *   C.Carman	new debugging idl functions
 */

#include <acquire/acqexception.h>
#include <control/orbutils.h>
#include <control/orbnaming.h>
#include "worklistimpl.h"

WorklistImpl*  WorklistImpl::_instance = NULL;
bool WorklistImpl::_isShuttingDown = false;


// Ask the ORB to stop nicely
void WorklistImpl::
shutdown()
{
	if ( _isShuttingDown )
		return;
	_isShuttingDown = true;

	if ( _instance )
		_instance->shutdownPortal();
}


// Invoke ORB run, does not return until someone has called shutdown.
void WorklistImpl::
run( const char *mergeIniFile, const char *worklistName, const char *facilityName, DictionaryPkg::VersionInfo * versionInfo )
{
	const char *exceptionMessage = "";

	/*
	 * static methods remembers what name we wanted
	 * when a connection is finally attempted.
	 */
	ConnectFacility::useName( facilityName);
	try
	{
		exceptionMessage = "cannot create WorklistImpl";
		_instance = new WorklistImpl( mergeIniFile, worklistName, versionInfo );

		// Toss the main worklist object in a separate POA
		exceptionMessage = "cannot add WorklistImpl to poa";
		PortableServer::POA_var poa = ORBUtils::createPersistentPOA( worklistName );
		PortableServer::ObjectId_var worklistId = PortableServer::string_to_ObjectId(worklistName);
		poa->activate_object_with_id(worklistId.in(), _instance);

		 // Register main worklist object with naming service
		exceptionMessage = "cannot register WorklistImpl with naming service";
		DICOMPkg::Worklist_var wl_var = _instance->_this();
		ORBUtils::namingServiceRebind( worklistName, wl_var.in());

		// Start up poa manager
		exceptionMessage = "cannot start poa manager";
		PortableServer::POAManager_var poa_manager = ORBUtils::getPOAManager();
		poa_manager->activate ();
	}
	catch( ... )
	{
		::Message( MALARM, MLoverall | toEndUser | toService | toDeveloper, "Cannot make Worklist Server available to network: %s.", exceptionMessage);
		delete _instance;
		_instance = NULL;
		throw;
	}

	::Message( MNOTE, MLoverall, "Worklist started." );

	try
	{
		exceptionMessage = "no orb";

		CORBA::ORB_var orb = ORBUtils::getORB();
		orb->run();
	}
	catch( const CORBA::Exception& e)
	{
		::Message( MALARM, MLoverall | toEndUser | toService | toDeveloper, "Worklist ORB terminated prematurely");
		sleep(1);
		throw;
	}
	::Message( MSTATUS, MLoverall, "Worklist is stopping, ORB processing complete." );

	try
	{
		delete _instance;
	}
	catch (...)
	{
		::Message( MNOTE, MLoverall, "WorklistImpl::run() ignoring exception while deleting _instance");
	}
	_instance = NULL;

	try
	{
		ORBUtils::destroy();
	}
	catch( const CORBA::Exception& e)
	{
		::Message( MALARM, MLoverall | toEndUser | toService | toDeveloper, "WorklistImpl::run() caught exception shutting down orb");
		sleep(1);
		throw;
	}

	_isShuttingDown = false;

	::Message( MSTATUS, MLoverall, "Worklist is stopping, ORB destroyed." );
}

WorklistImpl::WorklistImpl( const char *mergeIniFile, const char *worklistName, DictionaryPkg::VersionInfo * versionInfo ) :
		PortalImpl(worklistName)
{
	_worklistName = CORBA::string_dup(worklistName);
	_versionInfo = new DictionaryPkg::VersionInfo(*versionInfo);

    try {
         _manualWorklist = new ManualWorklist( mergeIniFile);
         _automatedWorklist = new AutomatedWorklist( mergeIniFile);

         g_ptrToAutomatedWorklist = _automatedWorklist;
         _automatedWorklist->runTimer();
    }
    catch( ...) {
	::Message( MALARM, MLoverall | toEndUser, "Cannot create the worklist object.");
        throw;
    }
}

void WorklistImpl::
deactivate()
{
	// deactivate main Worklist that is in a different POA
	PortableServer::POA_var poa = ORBUtils::getPOA(_worklistName);
	DICOMPkg::Worklist_ptr wp = _instance->_this();
	PortableServer::ObjectId_var obj =   poa->reference_to_id( wp );
	poa->deactivate_object( obj.in() );
	CORBA::release( wp );
}

WorklistImpl::~WorklistImpl() throw()
{
	Message( MSTATUS, "Worklist server stopping...");

	delete _manualWorklist;
	delete _automatedWorklist;
}

StationPkg::Session_ptr WorklistImpl::
startSession( StationPkg::Station_ptr sptr, DictionaryPkg::UserType userType)
throw( DictionaryPkg::NucMedException)
{
	return NULL;
}

CORBA::Boolean WorklistImpl::setWorklistProtocolMapping(
        const DICOMPkg::ProtocolMapList& mapping
        ) throw (CORBA::SystemException)
{	try {
		::Message( MNOTE, toEndUser | toService | MLoverall, 
                   "The Worklist Server is called to set a new Protocol Map");
		_manualWorklist->setProtocolMapping(mapping);
		_automatedWorklist->setProtocolMapping(mapping);
	}
	catch(...) {
		return false;
	}

	return true;
}

CORBA::Long WorklistImpl::getWorkListMatch(
        const DictionaryPkg::LookupMatchList& toMatch
        ) throw (CORBA::SystemException)
{
	return _manualWorklist->sendWorklistQuery(toMatch);
}

CORBA::Long WorklistImpl::getTodaysWorkList() throw (CORBA::SystemException)
{
	throw "WorklistImpl::getTodaysWorkList: Not implemented!";
}

CORBA::Boolean WorklistImpl::isScheduledVisitStillValid(
        const PatientPkg::ScheduledVisit& toMatch
        ) throw (CORBA::SystemException)
{
	throw "WorklistImpl::isScheduledVisitStillValid: Not implemented!";
}

CORBA::Boolean WorklistImpl::setAutomatedWorklistQueryDefaults(
        const DictionaryPkg::LookupMatchList& defaults
        ) throw (CORBA::SystemException)
{	try {
		_automatedWorklist->setQueryDefaults(defaults);
	}
	catch(...) {
		return false;
	}

	return true;
}

void WorklistImpl::
remoteDebug( const char *action )
{
	// do something ourselves based on string command in "action"

	// optionally call one from portalimpl (may not always want/need to call this one)
	PortalImpl::remoteDebug(action);
}

WorklistImpl &WorklistImpl::
operator=( const WorklistImpl &)
{
	const char * exceptionMessage = "Don't use WorklistImpl operator=";
	Message( MALARM, MLproblem, exceptionMessage );
	throw AcqExceptionNotImplemented( exceptionMessage );
}

WorklistImpl::
WorklistImpl( const WorklistImpl &) :
  PortalImpl( DICOMPkg::Worklist::COMPONENT_LABEL )
{
	const char * exceptionMessage = "Don't use WorklistImpl copy constructor";
	Message( MALARM, MLproblem, exceptionMessage );
	throw AcqExceptionNotImplemented( exceptionMessage );
}
