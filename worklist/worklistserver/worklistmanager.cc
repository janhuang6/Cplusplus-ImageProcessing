/*
 * file:	worklistmanager.cc
 * purpose:	Implementation of the WorklistManager class
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */

#include <unistd.h>
#include <stdlib.h>
#include <thread.h>
#include <ctype.h>
#include <math.h>
#include <iostream.h>

#include "worklistmanager.h"
#include "control/lookupmatchutils.h"
#include "control/stationimpl.h"

ThreadMutex g_lock;   /* Serialize access to the critical section.*/

int WorklistManager::_applicationID = -1; /* initialize the static member, one per app */

static const char rcsID[] = "$Id: worklistmanager.cc,v 1.69 jhuang Exp $";
static const char rcsName[] = "$Name: Atlantis710R01_branch $";
extern const char *worklistBuildDate;

static const char *userPW = "empty";

#ifdef MYDEBUG
void 
WorklistManager::printTraitList(const DictionaryPkg::TraitValueList &traits)
{
  char tmpStr[256];
  int listLen = traits.length();
  if( 0 == listLen ) {
	cerr << "\tEmpty trait value list." << endl;
	return;
  }

  try {
    for( int i=0; i< listLen; ++i) {
	  _ptrToTraits->getTraitValue( traits[i], tmpStr, 255);
	  cerr << "\t" << _ptrToTraits->getName( traits[i].tid ) << ": " << tmpStr << endl;
    }
  } catch (... ) {
	cerr << "Error printing trait value list!!!" << endl;
  }
}
#endif

WorklistManager::WorklistManager(const char *mergeIniFile)
{
  char hostname[256];
  int iSleepCount=0;
  int timeout;           /* Timeout value                    */
  int threshold;         /* Reply message cut-off threshold  */
  StationImpl *station = NULL;
  DictionaryPkg::SystemInfoList * systems;
  DictionaryPkg::SystemInfo *curSystem;
  DictionaryPkg::AppServiceList *services;
  DictionaryPkg::TraitValueList *serviceTraits;
  DictionaryPkg::TraitValueList *serviceConfTraits;
  DictionaryPkg::TraitValue serviceTypeValue;
  DictionaryPkg::TraitValue serviceProtocolValue;
  DictionaryPkg::TraitValue serviceHostValue;
  char systemName[64]; /* used pointer caused core dump */
  const char *serviceType = NULL;
  const char *serviceProtocol = NULL;
  const char *serviceHost = NULL;
  int traitID;
  DictionaryPkg::TraitValue servicePortValue;
  DictionaryPkg::TraitValue serviceCalledAEValue;
  DictionaryPkg::TraitValue serviceCallingAEValue;
  const char *servicePort = NULL;
  const char *serviceCalledAETitle = NULL;
  const char *serviceCallingAETitle = NULL;
  char WorklistProviderDefined = 0;
  DictionaryPkg::TraitValueList_var facilityInfo = NULL;
  char isWorklistLicensed[8] ;

  _queryID = 0;

  _myVisit = NULL;
  _ptrToWorklistUtils = NULL;
  _ptrToTraits = NULL;
  _myMatchUtils = NULL;

  _session = NULL;
  _dictionary = NULL;
  _connection = NULL;

// These lines of code are modified from testConnectFacilitySession.cc
  try {
    Message( MNOTE, MLoverall | toService | toDeveloper, "Worklist is attempting to connect to Facility.");
    gethostname(hostname,sizeof(hostname));
    Message( MNOTE, MLoverall | toService | toDeveloper, "Setting hostname as %s", hostname);
//    ConnectFacility::useName(hostname);
    ConnectFacility::useName("ADAC01");
    //ConnectFacility::setUserType(DictionaryPkg::USER_INTERNAL);
    _connection = ConnectFacility::instance();
  } catch (... ) {
    Message( MWARNING, MLoverall | toService | toDeveloper, "Failed to connect to Facility.");
    throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
  }

  Message( MNOTE, MLoverall | toService | toDeveloper, "Waiting for connection --");
  while (0==_connection->isReady()){ 
    sleep(2);
    iSleepCount +=2;
    Message( MNOTE, MLoverall | toService | toDeveloper, "\b\b%2d",iSleepCount);
    if (iSleepCount > 60){
      Message( MWARNING, MLoverall | toService | toDeveloper, "Connection timeout");
      throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
    }
  }
  Message( MNOTE, MLoverall | toService | toDeveloper, "\n");

  station = NULL;
  try {
    Message( MNOTE, MLoverall | toService | toDeveloper, "Creating StationImpl");
    station = new StationImpl( DICOMPkg::Worklist::COMPONENT_NAME, userPW, componentName, rcsName, worklistBuildDate );
	if( station == NULL)
          throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
  } catch (... ) {
      Message( MWARNING, MLoverall | toService | toDeveloper, "Failed to create a Station");
      throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
  }

  try {
    Message( MNOTE, MLoverall | toService | toDeveloper, "Attempting to get a Facility Session");
    _session = _connection->getSession( station, DictionaryPkg::USER_INTERNAL );
// The following line trying to fix the memory leak, caused core dump.
//	if(station) { delete station; station = NULL; }
  } catch (... ) {
      Message( MWARNING, MLoverall | toService | toDeveloper, "Failed to get a Facility Session");
      throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
  }
  Message( MNOTE, MLoverall | toService | toDeveloper, "Successful in getting Facility Session");

  try {
        Message( MNOTE, MLoverall | toService | toDeveloper, "Attempting to get Trait Dictionary");
        _dictionary = _connection->getTraits();
  } catch (... ) {
    Message( MWARNING, MLoverall | toService | toDeveloper, "Failed to get Trait Dictionary");
    throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
  }

  if(_dictionary != NULL && _dictionary->length() != 0)
     Message( MNOTE, MLoverall | toService | toDeveloper, "Successful in getting Trait Dictionary");
  else {
     Message( MWARNING, MLoverall | toService | toDeveloper, "Got null pointer for the Trait Dictionary or the length is zero");
     throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
  }

  try {
        Message( MNOTE, MLoverall | toService | toDeveloper, "Creating an instance of TraitUtils");
        _ptrToTraits = new TraitUtils(_dictionary);
  } catch (... ) {
    Message( MWARNING, MLoverall | toService | toDeveloper, "Failed to instantiate TraitUtils");
    throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
  }

  Message( MNOTE, MLoverall | toService | toDeveloper, "Successful in instantiating a TraitUtils");

  facilityInfo = _connection->getFacilityInfo();
  memset(isWorklistLicensed,0,sizeof(isWorklistLicensed));
  _ptrToTraits->getTraitValue( "TRAIT_FACL_IS_WORKLIST_LICENSED", facilityInfo, isWorklistLicensed, sizeof(isWorklistLicensed));
  if (0!=strcasecmp(isWorklistLicensed,"true")){
    Message( MWARNING, MLoverall | toService | toDeveloper, "Worklist is not licensed. Exit");
    sleep(5);  // hang around just long enough that the startup scripts see we have started
	throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
  }

  Message( MNOTE, MLoverall | toService | toDeveloper, "Successfully getting the Worklist License");

  try {
        Message( MNOTE, MLoverall | toService | toDeveloper, "Creating an instance of LookupMatchUtils");
        _myMatchUtils = new LookupMatchUtils(*_ptrToTraits);
  } catch (... ) {
        Message( MWARNING, MLoverall | toService | toDeveloper, "Failed to instantiate LookupMatchUtils");
        throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
  }
  Message( MNOTE, MLoverall | toService | toDeveloper, "Successful in instantiating a LookupMatchUtils");

  try {
        Message( MNOTE, MLoverall | toService | toDeveloper, "Creating an instance of VisitUtils");
        _myVisit = new VisitUtils(*_ptrToTraits);
  } catch (... ) {
        Message( MWARNING, MLoverall | toService | toDeveloper, "Failed to instantiate VisitUtils");
        throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
  }
  Message( MNOTE, MLoverall | toService | toDeveloper, "Successful in instantiating a VisitUtils");

  MutexGuard guard (_lockProtocolMapList);
// These lines of code are modified from testConnectFacility.cc
  try {
        Message( MNOTE, MLoverall | toService | toDeveloper, "Attempting to get the Worklist Protocol Mapping");
	DictionaryPkg::TraitMapList_var mapList = _connection->getWorklistProtocolMapping();
	_myProtocolMapList = mapList.in();
  } catch (... ) {
        Message( MWARNING, MLoverall | toService | toDeveloper, "Exception while getting the worklist protocol mapping");
        throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
  }
  guard.release();

// These lines of code are modified from acquisitionmanagerimpl.cc
// by changing "ImageReceiver" to be "WorklistProvider"

// now get the SystemInfoList

  if(( systems = _connection->getSystemsInfo()) == NULL)
     {
       Message( MWARNING, "Cannot access sytem information from data server.");
       throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
     }

// search through the systems list for the requested save location

  for (int i=0; i< (int)systems->length(); i++)
     {
       curSystem = &(*systems)[i];
       services = &(curSystem->services);
       if (services->length() == 0)
           continue;
       strncpy(systemName, curSystem->id.name, sizeof(systemName));
       systemName[sizeof(systemName)-1] = 0;
       Message( MNOTE, MLoverall | toService | toDeveloper, "Check system %s with %ld services", systemName, (long)services->length() );
       for (int j=0; j<(int)services->length(); j++)
          {
            serviceTraits = &(*services)[j].svcInfo;
            traitID = _ptrToTraits->getID("TRAIT_APPS_SERVICE_TYPE"); 

            try {
                  serviceTypeValue = _ptrToTraits->getValue(traitID, *serviceTraits);
            }
            catch (...) {
                  Message( MWARNING, MLoverall | toService | toDeveloper, "Service type trait %d not found", traitID);
                  break;
            }
            serviceTypeValue.tval >>= serviceType;
            if (strcmp(serviceType, "WorklistProvider") != 0)
                continue;

			WorklistProviderDefined = 1;
			try {
                  serviceProtocolValue =  _ptrToTraits->getValue("TRAIT_APPS_SERVICE_PROTOCOL", *serviceTraits);
            }
            catch (...) {
                  Message( MWARNING, MLoverall | toService | toDeveloper, "No protocol defined for a Worklist Provider service!");
                  throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
            }
            serviceProtocolValue.tval >>= serviceProtocol;

            serviceConfTraits = &(*services)[j].svcConfig;

            try {
                  serviceHostValue = _ptrToTraits->getValue("TRAIT_APPS_CONF_HOST", *serviceConfTraits);
                  serviceHostValue.tval >>= serviceHost;
            }
            catch (...) {
                  Message( MWARNING, MLoverall | toService | toDeveloper, "No host defined for Worklist Provider service! Will use system name instead");
                  serviceHost = systemName;
            }

            if (strcmp(serviceProtocol, "DICOM") == 0)
            {
            try {
                  servicePortValue = _ptrToTraits->getValue("TRAIT_APPS_CONF_PORT", *serviceConfTraits);
                  servicePortValue.tval >>= servicePort;
            }
            catch (...) {
                  Message( MWARNING, MLoverall | toService | toDeveloper, "No port number defined for DICOM Worklist Provider service! Program cannot continue.");
                  exit(1);
            }

            try {
                  serviceCalledAEValue = _ptrToTraits->getValue("TRAIT_APPS_CONF_CALLED_AE", *serviceConfTraits);
                  serviceCalledAEValue.tval >>= serviceCalledAETitle;
            }
            catch (...) {
                  Message( MWARNING, MLoverall | toService | toDeveloper, "No Called AE Title defined for DICOM Worklist Provider service! Program cannot continue.");
                  exit(1);
            }

			memset(_sysLocalAETitle, 0, 64);
            try {
                  serviceCallingAEValue = _ptrToTraits->getValue("TRAIT_APPS_CONF_CALLING_AE", *serviceConfTraits);
                  serviceCallingAEValue.tval >>= serviceCallingAETitle;
                  strcpy(_sysLocalAETitle, serviceCallingAETitle);
				  if(strlen(serviceCallingAETitle)==0)
					  serviceCallingAETitle = "PMS_WORK_SCU";
            }
            catch (...) {
                  Message( MWARNING, MLoverall | toService | toDeveloper, "No Calling AE Title defined for DICOM Worklist Provider service! Will use default \"PMS_WORK_SCU\" to initialize the Merge Library.");
                  serviceCallingAETitle = "PMS_WORK_SCU"; // Use "" will cause PerformInitialization: Unable to register this application with the library
            }

            Message( MNOTE, MLoverall | toService | toDeveloper, "Found DICOM system: %s", systemName );
            break;
         }
      }
  }

  if(WorklistProviderDefined == 0) {
	  Message( MWARNING, MLoverall | toService | toDeveloper, "No DICOM Worklist Provider service defined! Program cannot continue.");
	  exit(1);
  }

  if (_applicationID == -1) {
        if(mergeIniFile==NULL || strlen(mergeIniFile) == 0) {
            Message( MNOTE, MLoverall | toService | toDeveloper, "#Software error: Merge INI file is NULL.");
            throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
        }

        if(MERGE_FAILURE == PerformMergeInitialization( mergeIniFile, &_applicationID, serviceCallingAETitle )) {
            Message( MNOTE, MLoverall | toService | toDeveloper, "#Software error: Cannot initialize the Merge Toolkit.");
            throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
        }
  }

  // default value
  timeout = 10000;
  threshold = 1000;

  if(serviceCalledAETitle != NULL && servicePort != NULL && serviceHost != NULL)
  {
    char str[256];
    snprintf(str, 255, "Get serviceCalledAETitle = %s, servicePort = %s, and serviceHost = %s from the Data Server", serviceCalledAETitle, servicePort, serviceHost);
    // Has to get everything in str before calling the Message.
    // If pass arguments to Message, the worklist server core
    // dumps in a strange way.
    Message( MNOTE, MLoverall | toService | toDeveloper, str);
     _ptrToWorklistUtils = new WorklistUtils(serviceCalledAETitle,
                                            atoi(servicePort), serviceHost,
                                            timeout, threshold);
  }
  else 
  {
    Message( MWARNING, MLoverall | toService | toDeveloper, "Got invalid parameters from WorklistProvider service and could not construct WorklistUtils");
    throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
  }
}

WorklistManager::~WorklistManager() throw()
{
    delete _myVisit;
    delete _ptrToWorklistUtils;
    delete _ptrToTraits;
    delete _myMatchUtils;

	delete _session;
	delete _dictionary;
// connectFacility only defines a private destructor
//	delete _connection;

    /*
    ** The last thing that we do is to release this application from the
    ** library.
    */
    MC_Release_Application( &_applicationID );

    if (MC_Library_Release() != MC_NORMAL_COMPLETION)
		Message( MALARM, toEndUser | toService | MLoverall, "Error releasing the library.");
}

void WorklistManager::setProtocolMapping(const DICOMPkg::ProtocolMapList& mapping)
{
        MutexGuard guard (_lockProtocolMapList);
        _myProtocolMapList = mapping;
        guard.release();
}

void* WorklistManager::_sendWorklistQuery(void* thisClass)
{
	// First check the license is still valid
	WorklistManager *ptrToWorklist;
	DictionaryPkg::TraitValueList_var facilityInfo = NULL;
	char isWorklistLicensed[8] ;

	ptrToWorklist = (WorklistManager*)thisClass;
	facilityInfo = ptrToWorklist->_connection->getFacilityInfo();
	memset(isWorklistLicensed,0,sizeof(isWorklistLicensed));
	ptrToWorklist->_ptrToTraits->getTraitValue( "TRAIT_FACL_IS_WORKLIST_LICENSED", facilityInfo, isWorklistLicensed, sizeof(isWorklistLicensed));
	if (0!=strcasecmp(isWorklistLicensed,"true")){
		::Message( MWARNING, MLoverall | toService | toDeveloper, "Worklist license has been disabled since server started. Now block the worklist query.");
		pthread_exit( NULL );
	}

	MutexGuard guard (g_lock);

	QueryPara qpar;
	int len;
	MC_STATUS status;        /* The status returned via toolkit */
	PatientPkg::ScheduledVisitList *svlist=NULL;
	QueryStatus query_status = DictionaryPkg::QUERYSTATUS_FAILURE;
	QueryType query_type;

	query_type = ptrToWorklist->getQueryType();
	if( query_type == DictionaryPkg::QUERYTYPE_MANUAL )
		ptrToWorklist->_queryID++;
	else
		ptrToWorklist->_queryID--;

	::Message( MNOTE, toEndUser | toService | MLoverall, 
                   "Start worklist query ID = %ld",  ptrToWorklist->_queryID);
#ifdef MYDEBUG
printf("Start worklist query ID = %ld\n",  ptrToWorklist->_queryID);
#endif

	memset((void*) &qpar, 0, sizeof(QueryPara));
	try {
		ptrToWorklist->processQueryParameter(qpar);
	}
	catch (...) {
		// return the queryStatus
		ptrToWorklist->_session->worklistQueryStatus(query_status, query_type, ptrToWorklist->_queryID);

		::Message( MALARM, toEndUser | toService | MLoverall, 
		   	   "processQueryParameter failed.");

		guard.release();
		pthread_exit( NULL );
	}

	if( strlen(qpar.scheduledProcStepStartDate)==0 )
		ptrToWorklist->getQueryDates(qpar.scheduledProcStepStartDate);

        ::Message( MNOTE, toEndUser | toService | MLoverall, 
                   "Query for dates: %s", qpar.scheduledProcStepStartDate);
#ifdef MYDEBUG
printf("Query for dates: %s\n", qpar.scheduledProcStepStartDate);
#endif

        /*
        ** Attempt to open an association with the provider.
        */
        try {
              ptrToWorklist->_associationID = ptrToWorklist->_ptrToWorklistUtils->openAssociation( ptrToWorklist->_applicationID );
        }
        catch (...) {
             // return the queryStatus
             ptrToWorklist->_session->worklistQueryStatus(query_status, query_type, ptrToWorklist->_queryID);

             /*
             ** We were unable to open the association
             ** with the server and decide to quit.
             */
             ::Message( MALARM, toEndUser | toService | MLoverall, 
		   	"Open association failed. Check the SCP is running and the hostname and port number are correct and try again.");
#ifdef MYDEBUG
printf("Open association failed. Check the SCP is running and the hostname and port number are correct and try again.\n");
#endif
             guard.release();
             pthread_exit( NULL );
        }
	
        /*
        ** At this point, the user has filled in all of their
        ** message parameters, and then decided to query.
        ** Therefore, we need to construct and send their message
        ** off to the server.
        */
        try {
	        ::Message( MNOTE, toEndUser | toService | MLoverall, "Query Parameters are:");
	        ::Message( MNOTE, toEndUser | toService | MLoverall, "callingAeTitle: \"%s\"", qpar.localAeTitle);
	        ::Message( MNOTE, toEndUser | toService | MLoverall, "scheduledProcStepStartDate: \"%s\"", qpar.scheduledProcStepStartDate);
	        ::Message( MNOTE, toEndUser | toService | MLoverall, "scheduledProcStepStartTime: \"%s\"", qpar.scheduledProcStepStartTime);
	        ::Message( MNOTE, toEndUser | toService | MLoverall, "modality: \"%s\"", qpar.modality);
	        ::Message( MNOTE, toEndUser | toService | MLoverall, "scheduledPerformingPhysicianName: \"%s\"", qpar.scheduledPerformPhysicianName);
	        ::Message( MNOTE, toEndUser | toService | MLoverall, "accessionNumber: \"%s\"", qpar.accessionNumber);
	        ::Message( MNOTE, toEndUser | toService | MLoverall, "referringPhysician: \"%s\"", qpar.referringPhysician);
	        ::Message( MNOTE, toEndUser | toService | MLoverall, "patientName: \"%s\"", qpar.patientName);
	        ::Message( MNOTE, toEndUser | toService | MLoverall, "patientID: \"%s\"", qpar.patientID);
#ifdef MYDEBUG
printf("Query Parameters are: \n");
printf("\tcallingAeTitle: \"%s\"\n", qpar.localAeTitle);
printf("\tscheduledProcStepStartDate: \"%s\"\n", qpar.scheduledProcStepStartDate);
printf("\tscheduledProcStepStartTime: \"%s\"\n", qpar.scheduledProcStepStartTime);
printf("\tmodality: \"%s\"\n", qpar.modality);
printf("\tscheduledPerformingPhysicianName: \"%s\"\n", qpar.scheduledPerformPhysicianName);
printf("\taccessionNumber: \"%s\"\n", qpar.accessionNumber);
printf("\treferringPhysician: \"%s\"\n", qpar.referringPhysician);
printf("\tpatientName: \"%s\"\n", qpar.patientName);
printf("\tpatientID: \"%s\"\n", qpar.patientID);
#endif

              ptrToWorklist->_ptrToWorklistUtils->sendWorklistQuery( ptrToWorklist->_associationID, qpar );
        }
        catch (...) {
             // return the queryStatus
             ptrToWorklist->_session->worklistQueryStatus(query_status, query_type, ptrToWorklist->_queryID);

             ::Message( MALARM, toEndUser | toService | MLoverall, "Query failed.");
             /*
             ** Before exiting, we will attempt to close the association
             ** that was successfully formed with the server.
             */
             MC_Abort_Association( &(ptrToWorklist->_associationID) );

             guard.release();
             pthread_exit( NULL );
        }

        /*
        ** We need to create an empty list that will hold our
        ** query results.
        */
        if ( LLCreate( &(ptrToWorklist->_patientList),
                       sizeof( WorkPatientRec ) ) != MERGE_SUCCESS )
        {
             // return the queryStatus
             ptrToWorklist->_session->worklistQueryStatus(query_status, query_type, ptrToWorklist->_queryID);

             ::Message( MALARM, toEndUser | toService | MLoverall,
      	              "Unable to initialize the linked list. worklistmanager.cc line %d",
                      __LINE__);
             /*
             ** This is a rather pesky situation.  We cannot continue
             ** if we are unable to allocate memory for the linked list.
             ** We will, however attempt to abort this association
             ** from the toolkit. 
             */
             MC_Abort_Association( &(ptrToWorklist->_associationID) );

             guard.release();
             pthread_exit( NULL );
         }

        /*
        ** We've now sent out a CFIND message to a service class
        ** provider.  We will now wait for a reply from this server,
        ** and then "process" the reply.
        */
        try {
              ptrToWorklist->_ptrToWorklistUtils->processWorklistReplyMsg( 
                                ptrToWorklist->_associationID, 
                                &(ptrToWorklist->_patientList) );
        }
        catch (...) {
             // return the queryStatus
             ptrToWorklist->_session->worklistQueryStatus(query_status, query_type, ptrToWorklist->_queryID);

             ::Message( MALARM, toEndUser | toService | MLoverall, 
		   	             "Processing Reply Message failed.");
             LLDestroy( &(ptrToWorklist->_patientList) );
             MC_Abort_Association( &(ptrToWorklist->_associationID) );

             guard.release();
             pthread_exit( NULL );
        }

        /*
        ** Now that we are done, we'll close the association that
        ** we've formed with the server.
        */
        status = MC_Close_Association( &(ptrToWorklist->_associationID) );
        if ( status != MC_NORMAL_COMPLETION )
        {
               // return the queryStatus
             ::Message( MALARM, toEndUser | toService | MLoverall,
                       "Unable to close the association with HIS/RIS. worklistmanager.cc line %d",
                       __LINE__);
        }

        /*
        ** If we got no patients back from the server, tell the
        ** user that.
        */
        len = LLNodes( &(ptrToWorklist->_patientList) );

	    /*
        ** Print the list of patients here.
        */
		if (len == 0) {
            ::Message( MNOTE, toEndUser | toService | MLoverall, "Query returns successfully with no worklist entry.");
#ifdef MYDEBUG
			printf( "Query returns successfully with no worklist entry.\n");
#endif
		}
		else {
            ::Message( MNOTE, toEndUser | toService | MLoverall, "Query results are:");
            ::Message( MNOTE, toEndUser | toService | MLoverall, "      %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s", "PatientName", "PatientID", "PatientDOB", "Height", "Weight", "Gender", "PatientState", "PregnancyStatus", "MedicalAlerts", "Allergies", "SpecialNeeds", "LocalAETitle", "StartDate", "StartTime", "Modality", "PerformingPhysician", "StepDesc", "StationName", "StepID", "schdProtCodeValue", "schdProtCodingScheme", "schdProtCodeMeaning", "ProcID", "ProcDesc", "ReqProcCodeMeaning", "StudyInstUID", "Accession#", "RequestingPhysician", "ReferringPhysician", "otherPatientID", "intendedRecipients", "imageServiceRequestComments", "requestedProcComments", "seriesInstanceUID", "reason4RequestProc", "refSOPClassUID", "refSOPInstUID");
            ::Message( MNOTE, toEndUser | toService | MLoverall, "      %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------" );
#ifdef MYDEBUG
			printf( "Query results are:\n");
			printf( "      %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s\n", "PatientName", "PatientID", "PatientDOB", "Height", "Weight", "Gender", "PatientState", "PregnancyStatus", "MedicalAlerts", "Allergies", "SpecialNeeds", "LocalAETitle", "StartDate", "StartTime", "Modality", "PerformingPhysician", "StepDesc", "StationName", "StepID", "schdProtCodeValue", "schdProtCodingScheme", "schdProtCodeMeaning", "ProcID", "ProcDesc", "ReqProcCodeMeaning", "StudyInstUID", "Accession#", "RequestingPhysician", "ReferringPhysician", "otherPatientID", "intendedRecipients", "imageServiceRequestComments", "requestedProcComments", "seriesInstanceUID", "reason4RequestProc", "refSOPClassUID", "refSOPInstUID");
			printf( "      %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s\n", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------" );
#endif
		}

        try {
            svlist = new PatientPkg::ScheduledVisitList(len);
            ptrToWorklist->processReturnListInfo(svlist);
        }
        catch (...) {
            // return the queryStatus
            ptrToWorklist->_session->worklistQueryStatus(query_status, query_type, ptrToWorklist->_queryID);
            ::Message( MALARM, toEndUser | toService | MLoverall,
                       "processReturnListInfo failed." );
            LLDestroy( &(ptrToWorklist->_patientList) );
			if(svlist) { delete svlist; svlist = NULL; }

            guard.release();
            pthread_exit( NULL );
        }

        /*
        ** Now destroy the patient list, because we will reuse
        ** it later...
        */
        LLDestroy( &(ptrToWorklist->_patientList) );

	try {
#ifdef MYDEBUG
// The next few lines are only for debug purpose
		for(unsigned int g=0; g<svlist->length(); g++) {
			ptrToWorklist->printTraitList((*svlist)[g].personal);
			ptrToWorklist->printTraitList((*svlist)[g].schedule);
			ptrToWorklist->printTraitList((*svlist)[g].biometric);
			ptrToWorklist->printTraitList((*svlist)[g].location);
		}
#endif

		ptrToWorklist->_session->importScheduledVisitList(*svlist);
		if(svlist) { delete svlist; svlist = NULL; }
	}
	catch (...) {
		// return the queryStatus
		ptrToWorklist->_session->worklistQueryStatus(query_status, query_type, ptrToWorklist->_queryID);

		::Message( MWARNING, MLoverall | toService | toDeveloper, "Cannot send ScheduledVisitList to the data server.");

		guard.release();
		pthread_exit( NULL );
	}

	// Eventually we succeeded. Return the queryStatus
	query_status = DictionaryPkg::QUERYSTATUS_SUCCESS;
	ptrToWorklist->_session->worklistQueryStatus(query_status, query_type, ptrToWorklist->_queryID);

	::Message( MNOTE, toEndUser | toService | MLoverall, 
                   "Query ID = %ld finished successfully.", ptrToWorklist->_queryID);

	guard.release();
	return NULL;
}

void WorklistManager::processQueryParameter(QueryPara &qpar)
{
	char firstname[64], lastname[64];
        const char *pc;
	char traitName[128];
	char startdate[16], enddate[16];
	double loEnd, hiEnd;

	memset(firstname, 0, sizeof(firstname));
	memset(lastname, 0, sizeof(lastname));

        MutexGuard guard (_lockLookupMatchList);
	for(int i=0; i<(int)_myLookupMatchList.length(); i++)
	{	memset(traitName, 0, sizeof(traitName));

		try {
                        pc=_ptrToTraits->getName(_myLookupMatchList[i].field);
                }
		catch (...) {
			::Message( MALARM, toEndUser | toService | MLoverall, "TraitID %ld not found in TraitAttrList.", (long)_myLookupMatchList[i].field);
			continue;
		}
                if(pc==NULL) {
			::Message( MALARM, toEndUser | toService | MLoverall, "TraitUtils::getName(%ld) returns NULL pointer.", (long)_myLookupMatchList[i].field);
			continue;
		}
                else
			strcpy(traitName, pc);

		if(!strcmp(traitName, "TRAIT_ACQR_MODALITY"))
		{	// This trait is for the Modality
			pc = _myMatchUtils->getString(_myLookupMatchList[i]);
			if( pc==NULL )
				::Message( MALARM, toEndUser | toService | MLoverall, 
					 "Unable to extract the value from TraitMatch _myLookupMatchList[%d].",
					 i);
			else
				strcpy(qpar.modality, pc);
		}
		else if(!strcmp(traitName, "TRAIT_APPS_CONF_CALLING_AE"))
		{	// This trait is for the Scheduled Station AE Title
			pc = _myMatchUtils->getString(_myLookupMatchList[i]);
			if( pc==NULL )
				::Message( MALARM, toEndUser | toService | MLoverall, 
					 "Unable to extract the value from TraitMatch _myLookupMatchList[%d].",
					 i);
			else
				strcpy(qpar.localAeTitle, pc);
		}
		else if(!strcmp(traitName, "TRAIT_SCHD_WHEN"))
		{	// This trait is for the Scheduled Procedure Step Start Date
			if( 0==_myMatchUtils->getDates(_myLookupMatchList[i], startdate, enddate, sizeof(enddate)))
				::Message( MALARM, toEndUser | toService | MLoverall, 
					 "Unable to extract the value from TraitMatch _myLookupMatchList[%d].",
					 i);
			else if(strlen(startdate) > 0 || strlen(enddate) > 0)
                        {
                                startdate[8]='\0';
                                enddate[8]='\0';
				sprintf(qpar.scheduledProcStepStartDate, "%s-%s", startdate, enddate);
                        }
/*
**                      A date string must be in the form of:
**                      YYYYMMDD, YYYYMMDD-YYYYMMDD, -YYYYMMDD, or YYYYMMDD-
*/
		}
		else if(!strcmp(traitName, "TRAIT_SCHD_ATTENDING_PHYSICIAN")) // no performing physician trait
		{	// This trait is for the Scheduled Performing Physician's Name
			pc = _myMatchUtils->getString(_myLookupMatchList[i]);
			if( pc==NULL )
				::Message( MALARM, toEndUser | toService | MLoverall, 
					 "Unable to extract the value from TraitMatch _myLookupMatchList[%d].",
					 i);
			else
				strcpy(qpar.scheduledPerformPhysicianName, pc);
		}
		else if(!strcmp(traitName, "TRAIT_SCHD_ACCESSION_NUMBER"))
		{	// This trait is for the Accession Number
			pc = _myMatchUtils->getString(_myLookupMatchList[i]);
			if( pc==NULL )
				::Message( MALARM, toEndUser | toService | MLoverall, 
					 "Unable to extract the value from TraitMatch _myLookupMatchList[%d].",
					 i);
			else
				strcpy(qpar.accessionNumber, pc);
		}
		else if(!strcmp(traitName, "TRAIT_SCHD_REFERRING_PHYSICIAN"))
		{	// This trait is for the Referring Physician's Name
			pc = _myMatchUtils->getString(_myLookupMatchList[i]);
			if( pc==NULL )
				::Message( MALARM, toEndUser | toService | MLoverall, 
					 "Unable to extract the value from TraitMatch _myLookupMatchList[%d].",
					 i);
			else
				strcpy(qpar.referringPhysician, pc);
		}
		else if(!strcmp(traitName, "TRAIT_PERS_GIVENNAME"))
		{	// This trait is for patient first name
			pc = _myMatchUtils->getString(_myLookupMatchList[i]);
			if( pc==NULL )
				::Message( MALARM, toEndUser | toService | MLoverall, 
					 "Unable to extract the value from TraitMatch _myLookupMatchList[%d].",
					 i);
			else
				strcpy(firstname, pc);
		}
		else if(!strcmp(traitName, "TRAIT_PERS_SURNAME"))
		{	// This trait is for patient last name
			pc = _myMatchUtils->getString(_myLookupMatchList[i]);
			if( pc==NULL )
				::Message( MALARM, toEndUser | toService | MLoverall, 
					 "Unable to extract the value from TraitMatch _myLookupMatchList[%d].",
					 i);
			else
				strcpy(lastname, pc);
		}
		else if(!strcmp(traitName, "TRAIT_PERS_IDNUMBER"))
		{	// This trait is for patient's ID
			pc = _myMatchUtils->getString(_myLookupMatchList[i]);
			if( pc==NULL )
				::Message( MALARM, toEndUser | toService | MLoverall, 
					 "Unable to extract the value from TraitMatch _myLookupMatchList[%d].",
					 i);
			else
				strcpy(qpar.patientID, pc);
		}

		// other data
		else if(!strcmp(traitName, "TRAIT_MWKL_QUERY_RELATIVE_DAYS"))
		{	// This trait means to query for this number of days starting today
			if( 0==_myMatchUtils->getRange( _myLookupMatchList[i], &loEnd, &hiEnd) ||
			    fabs(loEnd-hiEnd) > 0.0001 )
			{
				Message( MALARM, toEndUser | toService | MLoverall, 
					 "Unable to extract a unique numeric value from TraitMatch _myLookupMatchList[%d].",
					 i);
				_numberOfDaysToQuery = 1; // default value
                        }
			else
				_numberOfDaysToQuery = (int) (loEnd + 0.5);
		}
	}

        guard.release();

	if (strlen(lastname)>0 || strlen(firstname)>0) {
		if (strlen(lastname)>0)
			strcpy(qpar.patientName, lastname);
		else
			strcpy(qpar.patientName, "*");
		strcat(qpar.patientName, "^");
		if (strlen(firstname)>0)
			strcat(qpar.patientName, firstname);
		else
			strcat(qpar.patientName, "*");
	}

	if (getQueryType()==DictionaryPkg::QUERYTYPE_MANUAL && strlen(qpar.localAeTitle)==0)
		strcpy(qpar.localAeTitle, _sysLocalAETitle);
}

void WorklistManager::getQueryDates(char* dates)
{
   // For automated query, the worklistserver will set up the dates
   time_t tim;
   struct tm* do_time;
   char startdate[16], enddate[16], *pchr;

   tim = time(0); // current time
   do_time = localtime(&tim);

   sprintf(startdate, "%4d%2d%2d", do_time->tm_year+1900, do_time->tm_mon+1, do_time->tm_mday);
   pchr = startdate;
   while (*pchr) {
              if(*pchr == ' ') // Set the blank to '0'
                 *pchr='0';
              pchr++;
   }

   if (_numberOfDaysToQuery == 1)
   {
      strcpy(dates, startdate);
      return;
   }

   tim += 3600*24*(_numberOfDaysToQuery-1);
   do_time = localtime(&tim);

   sprintf(enddate, "%4d%2d%2d", do_time->tm_year+1900, do_time->tm_mon+1, do_time->tm_mday);
   pchr = enddate;
   while (*pchr) {
              if(*pchr == ' ') // Set the blank to '0'
                 *pchr='0';
              pchr++;
   }

   if (_numberOfDaysToQuery > 0)
      sprintf(dates, "%s-%s", startdate, enddate);
   else
      sprintf(dates, "%s-%s", enddate, startdate);
}

void WorklistManager::processReturnListInfo(PatientPkg::ScheduledVisitList *visit)
{
        WorkPatientRec patient;
        char firstname[64], lastname[64];
        int count = 0;
		char atlantis_when[32];
		char atlantis_protocol[64], mapKey[LO_LEN+1];

        /*
        ** Rewind the list of patients so that we can display the
        ** lists contents.
        */
        LLRewind( &_patientList );

        /*
        ** Now, walk the list, getting each record from it.
        */
        while( LLPopNode( &_patientList, &patient ) != NULL )
        {	
		        patientRec2Atlantis(&patient);
                if( count < 3 ) {
                    ::Message( MNOTE, toEndUser | toService | MLoverall, "[%2d]  %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s", count, "patName", "patID", patient.patientsDOB, patient.patientsHeight, patient.patientsWeight, patient.patientsGender, patient.patientState, patient.pregnancyStatus, patient.medicalAlerts, patient.contrastAllergies, patient.specialNeeds, patient.scheduledStationLocalAETitle, patient.scheduledProcedureStepStartDate, patient.scheduledProcedureStepStartTime, patient.modality, patient.scheduledPerformingPhysiciansName, patient.scheduledProcedureStepDescription, patient.scheduledStationName, patient.scheduledProcedureStepID, patient.scheduledProtocolCodeValue, patient.scheduledProtocolCodingSchemeDesignator, patient.scheduledProtocolCodeMeaning, patient.requestedProcedureID, patient.requestedProcedureDescription, patient.requestedProcedureCodeMeaning, patient.studyInstanceUID, patient.accessionNumber, patient.requestingPhysician, patient.referringPhysician, patient.otherPatientID, patient.intendedRecipients, patient.imageServiceRequestComments, patient.requestedProcedureComments, patient.seriesInstanceUID, patient.reasonForRequestedProcedure, patient.referencedSOPClassUID, patient.referencedSOPInstanceUID);
#ifdef MYDEBUG
                    printf( "[%2d]  %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s ", count, patient.patientsName, patient.patientID, patient.patientsDOB, patient.patientsHeight, patient.patientsWeight, patient.patientsGender, patient.patientState, patient.pregnancyStatus, patient.medicalAlerts, patient.contrastAllergies, patient.specialNeeds, patient.scheduledStationLocalAETitle, patient.scheduledProcedureStepStartDate);
                    printf("%16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s ", patient.scheduledProcedureStepStartTime, patient.modality, patient.scheduledPerformingPhysiciansName, patient.scheduledProcedureStepDescription, patient.scheduledStationName, patient.scheduledProcedureStepID, patient.scheduledProtocolCodeValue, patient.scheduledProtocolCodingSchemeDesignator, patient.scheduledProtocolCodeMeaning, patient.requestedProcedureID, patient.requestedProcedureDescription, patient.requestedProcedureCodeMeaning );
                    printf("%16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s\n", patient.studyInstanceUID, patient.accessionNumber, patient.requestingPhysician, patient.referringPhysician, patient.otherPatientID, patient.intendedRecipients, patient.imageServiceRequestComments, patient.requestedProcedureComments, patient.seriesInstanceUID, patient.reasonForRequestedProcedure, patient.referencedSOPClassUID, patient.referencedSOPInstanceUID );
#endif
                }
				else if(count == 3) {
                    ::Message( MNOTE, toEndUser | toService | MLoverall, "More patient records omitted ....");
#ifdef MYDEBUG
                    printf("More patient records omitted ....\n");
#endif
				}

                /* Fill out the scheduledVisitList by
                **
                ** Patient's Name (0010,0010): "TRAIT_PERS_GIVENNAME", "TRAIT_PERS_SURNAME" and "TRAIT_PERS_FULLNAME"
                ** Patient's ID (0010,0020): "TRAIT_PERS_IDNUMBER"
                ** Patient's DOB (0010,0030): "TRAIT_PERS_BIRTHDAY"
                ** Patient's Height (0010,1020): "TRAIT_BIOM_HEIGHT_CM"
                ** Patient's Weight (0010,1030): "TRAIT_BIOM_WEIGHT_KG"
                ** Patient's Gender (0010,0040): "TRAIT_PERS_GENDER"
                ** Patient State (0038,0500): "TRAIT_BIOM_PATIENT_STATE"
                ** Pregnancy Status (0010,21C0): "TRAIT_BIOM_PREGNANCY_STATUS"
                ** Medical Alerts (0010,2000): "TRAIT_BIOM_MEDICAL_ALERTS"
                ** Contrast Allergies (0010,2110): "TRAIT_BIOM_CONTRAST_ALLERGIES"
                ** Special Needs (0038,0050): "TRAIT_BIOM_SPECIAL_NEEDS"
                ** Other Patient's ID (0010,1000): "TRAIT_PERS_OTHER_PATIENT_IDS"
                ** Scheduled Procedure Step Sequence (0040,0100)
                ** >Scheduled Station AE Title (0040,0001): "TRAIT_APPS_CONF_CALLING_AE"
                ** >Scheduled Procedure Step Start Date (0040,0002): "TRAIT_SCHD_WHEN"
                ** >Scheduled Procedure Step Start Time (0040,0003): "TRAIT_SCHD_WHEN"
                ** >Modality (0008,0060): "TRAIT_ACQR_MODALITY" 
                ** >Scheduled Performing Physician.s Name (= Attending Physician in JETStream display) (0040,0006): "TRAIT_SCHD_ATTENDING_PHYSICIAN"
                ** >Scheduled Procedure Step Description (0040,0007): "TRAIT_SCHD_PROCEDURE_STEP_DESCRIPTION"
                ** >Scheduled Station Name (0040,0010): "TRAIT_SCHD_STATION_NAME"
                ** >Scheduled Procedure Step ID (0040,0009): "TRAIT_SCHD_PROCEDURE_STEP_ID"
                ** >Scheduled Protocol Code Sequence (0040,0008)
                ** >>Code Meaning (0008,0104): "TRAIT_SCHD_PROTOCOL_CODE_MEANING
                ** Requested Procedure ID (0040,1001): "TRAIT_SCHD_REQUESTED_PROCEDURE_ID"
                ** Requested Procedure Description (0032,1060): "TRAIT_SCHD_REQUESTED_PROCEDURE_DESCRIPTION"
                ** Study Instance UID (0020,000D): "TRAIT_SCHD_STUDY_INSTANCE_UID"
                ** Accession Number (0008,0050): "TRAIT_SCHD_ACCESSION_NUMBER"
                ** Requesting Physician (0032,1032): "TRAIT_SCHD_REQUESTING_PHYSICIAN"
                ** Referring Physician's Name (0008,0090): "TRAIT_SCHD_REFERRING_PHYSICIAN"
                ** Requested Procedure Code Sequence (0032,1064)
                ** >Code Meaning (0008,0104): "TRAIT_SCHD_REQUESTED_PROCEDURE_CODE_MEANING
                ** Intended recipients (0040,1010): "TRAIT_SCHD_INTENDED_RECIPIENTS"
                ** Image Service request Comments  (0040,2400): "TRAIT_SCHD_IMAGE_SERVICE_REQUEST_COMMENTS"
                ** Requested Procedure Comments (0040,1400): "TRAIT_SCHD_REQUESTED_PROCEDURE_COMMENTS"
                ** Image Service Request Reason (0040,2001): "TRAIT_SCHD_IMAGE_SERVICE_REQUEST_REASON"
                ** Series Instance UID (0020,000E): "TRAIT_ACQR_SERIES_INSTANCE_UID"
                ** Reason for requested Procedure (0040,1002): "TRAIT_SCHD_REQUESTED_PROCEDURE_REASON"
                */

		_myVisit->clear();
		_myVisit->useScheduledVisit();

		lastname[0]='\0';
		firstname[0]='\0';
		// This function also checks for group delimiter "=" and if found, it cuts off there in patientsName
		patientName2Atlantis(patient.patientsName, lastname, firstname);

		try {
/* [I don't like this, but...] hard code the fact that a few traits
 * must be present because they are isRequired:
 *	TRAIT_PERS_SURNAME
 *	TRAIT_PERS_GIVENNAME
 *  TRAIT_PERS_FULLNAME
 *	TRAIT_PERS_IDNUMBER
 *	TRAIT_PERS_BIRTHDAY
 *	TRAIT_PERS_GENDER
 *	TRAIT_BIOM_HEIGHT_CM
 *	TRAIT_BIOM_WEIGHT_KG
 *	TRAIT_SCHD_ACCESSION_NUMBER
 *	TRAIT_SCHD_ACC_NUM_EDITABLE
 *	TRAIT_SCHD_IS_QA (I think just set this always to false!)
 *	TRAIT_SCHD_STATUS (I think this should be set to PENDING.)
 *	TRAIT_SCHD_WHEN
 *  Chuck will log a defect and have isRequired removed or the check
 *  for isRequired removed. After that, we can uncomment the 
 *  //                        if(strlen(firstname)>0)
 *  so we only insert those fields with values.
 */

//                        if(strlen(firstname)>0)
                           _myVisit->set("TRAIT_PERS_GIVENNAME", firstname);

//                        if(strlen(lastname)>0)
                           _myVisit->set("TRAIT_PERS_SURNAME", lastname);

                           _myVisit->set("TRAIT_PERS_FULLNAME", patient.patientsName);

//                        if(strlen(patient.patientID)>0)
                           _myVisit->set("TRAIT_PERS_IDNUMBER", patient.patientID);

//                        if(strlen(patient.patientsDOB)>0)
						   {   char tmp[16];
						       strcpy(tmp, patient.patientsDOB);
                               // Convert DOB from YYYYMMDD to YYYYMMDDHHMMSS
                               if(strlen(patient.patientsDOB)==8)
                                  strcat(tmp, "000000");
                               _myVisit->set("TRAIT_PERS_BIRTHDAY", tmp);
						   }

//                        if(strlen(patient.patientsHeight)>0)
                           _myVisit->set("TRAIT_BIOM_HEIGHT_CM", atol(patient.patientsHeight));

//                        if(strlen(patient.patientsWeight)>0)
                           _myVisit->set("TRAIT_BIOM_WEIGHT_KG", atol(patient.patientsWeight));

                        if(patient.patientsGender[0] == 'M')
                           _myVisit->set("TRAIT_PERS_GENDER", "male");
                        else if(patient.patientsGender[0] == 'F')
                           _myVisit->set("TRAIT_PERS_GENDER", "female");
                        else
                           _myVisit->set("TRAIT_PERS_GENDER", "unknown");

                        if(strlen(patient.patientState)>0)
                           _myVisit->set("TRAIT_BIOM_PATIENT_STATE", patient.patientState);

                        if(strlen(patient.pregnancyStatus)>0)
                           _myVisit->set("TRAIT_BIOM_PREGNANCY_STATUS", patient.pregnancyStatus);

                        if(strlen(patient.medicalAlerts)>0)
                           _myVisit->set("TRAIT_BIOM_MEDICAL_ALERTS", patient.medicalAlerts);

                        if(strlen(patient.contrastAllergies)>0)
                           _myVisit->set("TRAIT_BIOM_CONTRAST_ALLERGIES", patient.contrastAllergies);

                        if(strlen(patient.specialNeeds)>0)
                           _myVisit->set("TRAIT_BIOM_SPECIAL_NEEDS", patient.specialNeeds);

                        if(strlen(patient.otherPatientID)>0)
                           _myVisit->set("TRAIT_PERS_OTHER_PATIENT_IDS", patient.otherPatientID);

//                        if(strlen(patient.scheduledStationLocalAETitle)>0)
//                           _myVisit->set("TRAIT_APPS_CONF_CALLING_AE", patient.scheduledStationLocalAETitle);

                        sprintf(atlantis_when, "%s%s", patient.scheduledProcedureStepStartDate, patient.scheduledProcedureStepStartTime);
//                        if(strlen(atlantis_when)>0)
                           _myVisit->set("TRAIT_SCHD_WHEN", atlantis_when);

//                        if(strlen(patient.modality)>0)
//                           _myVisit->set("TRAIT_ACQR_MODALITY", patient.modality);

                        physicianName2Atlantis(patient.scheduledPerformingPhysiciansName);
                        if(strlen(patient.scheduledPerformingPhysiciansName)>0)
                           _myVisit->set("TRAIT_SCHD_ATTENDING_PHYSICIAN", patient.scheduledPerformingPhysiciansName);

                        _myVisit->set("TRAIT_SCHD_PROCEDURE_STEP_DESCRIPTION", patient.scheduledProcedureStepDescription);

                        if(strlen(patient.scheduledStationName)>0)
                           _myVisit->set("TRAIT_SCHD_STATION_NAME", patient.scheduledStationName);

                        if(strlen(patient.scheduledProcedureStepID)>0)
                           _myVisit->set("TRAIT_SCHD_PROCEDURE_STEP_ID", patient.scheduledProcedureStepID);

                        if(strlen(patient.scheduledProtocolCodeValue)>0)
                           _myVisit->set("TRAIT_SCHD_PROTOCOL_CODE_VALUE", patient.scheduledProtocolCodeValue);

						// Now map the scheduledProtocolCodeValue to internal protocol name
                        if( _myVisit->getNumProtocols() == 0) {
                           _myVisit->getScheduledVisit().prescribed.length(1);
                           _myVisit->useProtocol(0);
                        }
                        _myVisit->set("TRAIT_PROT_NAME", "Unnamed Protocol");
                        _myVisit->set("TRAIT_PROT_TYPE", "clinical");

						memset(mapKey, 0, sizeof(mapKey));
						strcpy(mapKey, patient.scheduledProcedureStepDescription); // Customers are using this
//						strcpy(mapKey, patient.scheduledProtocolCodeValue); // IHE wants this
                        if(strlen(mapKey)>0) {
                           protocolName2Atlantis(mapKey, atlantis_protocol, sizeof(atlantis_protocol));
						   if(strlen(atlantis_protocol)<=0) {
								_myVisit->set("TRAIT_PROT_NAME", mapKey);
							    if( count < 3) {
                                   ::Message( MALARM, toEndUser | toService | MLoverall,
                                           "ProtocolMapping could not map '%s' to internal protocol name. Make sure a map entry has been established for it ahead of time.", mapKey );
								}
						   }
                           else {
                                ProtocolPkg::ProtocolInfoList * protocolInfoListPtr;

                                protocolInfoListPtr = _connection->getProtocol( atlantis_protocol);
                                if( protocolInfoListPtr != NULL && protocolInfoListPtr->length() >= 1) {
                                    _myVisit->getScheduledVisit().prescribed = protocolInfoListPtr[0];
                                }
								else {
									_myVisit->set("TRAIT_PROT_NAME", mapKey);
									if( count < 3) {
										::Message( MALARM, toEndUser | toService | MLoverall,
                                           "getProtocol('%s') failed for worklist entry '%s'.", atlantis_protocol, mapKey );
									}
								}
                           }
                        }

                        if(strlen(patient.scheduledProtocolCodingSchemeDesignator)>0)
                           _myVisit->set("TRAIT_SCHD_PROTOCOL_CODING_SCHEME", patient.scheduledProtocolCodingSchemeDesignator);
                        
						if(strlen(patient.scheduledProtocolCodeMeaning)>0)
                           _myVisit->set("TRAIT_SCHD_PROTOCOL_CODE_MEANING", patient.scheduledProtocolCodeMeaning);

//						if(strlen(patient.referencedComponentSOPClassUID)>0)
//                           _myVisit->set("TRAIT_SCHD_REF_STUDY_COMPONENT_SOP_CLASS_UID", patient.referencedComponentSOPClassUID);

//						if(strlen(patient.referencedComponentSOPInstanceUID)>0)
//                           _myVisit->set("TRAIT_SCHD_REF_STUDY_COMPONENT_SOP_INSTANCE_UID", patient.referencedComponentSOPInstanceUID);

                        if(strlen(patient.requestedProcedureID)>0)
                           _myVisit->set("TRAIT_SCHD_REQUESTED_PROCEDURE_ID", patient.requestedProcedureID);

                        if(strlen(patient.requestedProcedureDescription)>0)
                           _myVisit->set("TRAIT_SCHD_REQUESTED_PROCEDURE_DESCRIPTION", patient.requestedProcedureDescription);

						if(strlen(patient.studyInstanceUID)>0)
                           _myVisit->set("TRAIT_SCHD_STUDY_INSTANCE_UID", patient.studyInstanceUID);

                        _myVisit->set("TRAIT_SCHD_ACCESSION_NUMBER", patient.accessionNumber);
                        if(strlen(patient.accessionNumber)>0)
                           _myVisit->set("TRAIT_SCHD_ACC_NUM_EDITABLE", (long)0);
                        else
                           _myVisit->set("TRAIT_SCHD_ACC_NUM_EDITABLE", (long)1);

                        // These 2 are here also because of isRequired. A better fix later
                        _myVisit->set("TRAIT_SCHD_IS_QA", (long)0);
                        _myVisit->set("TRAIT_SCHD_STATUS", "PENDING");

                        physicianName2Atlantis(patient.requestingPhysician);
                        if(strlen(patient.requestingPhysician)>0)
                           _myVisit->set("TRAIT_SCHD_REQUESTING_PHYSICIAN", patient.requestingPhysician);

                        physicianName2Atlantis(patient.referringPhysician);
                        if(strlen(patient.referringPhysician)>0)
                           _myVisit->set("TRAIT_SCHD_REFERRING_PHYSICIAN", patient.referringPhysician);

                        if(strlen(patient.requestedProcedureCodeMeaning)>0)
                           _myVisit->set("TRAIT_SCHD_REQUESTED_PROCEDURE_CODE_MEANING", patient.requestedProcedureCodeMeaning);

                        if(strlen(patient.intendedRecipients)>0)
                           _myVisit->set("TRAIT_SCHD_INTENDED_RECIPIENTS", patient.intendedRecipients);

                        if(strlen(patient.imageServiceRequestComments)>0)
                           _myVisit->set("TRAIT_SCHD_IMAGE_SERVICE_REQUEST_COMMENTS", patient.imageServiceRequestComments);

                        if(strlen(patient.requestedProcedureComments)>0)
                           _myVisit->set("TRAIT_SCHD_REQUESTED_PROCEDURE_COMMENTS", patient.requestedProcedureComments);

                        if(strlen(patient.seriesInstanceUID)>0 && _myVisit->getNumProtocolSteps() > 0)
                           _myVisit->set("TRAIT_ACQR_SERIES_INSTANCE_UID", patient.seriesInstanceUID);

                        if(strlen(patient.reasonForRequestedProcedure)>0)
                           _myVisit->set("TRAIT_SCHD_REQUESTED_PROCEDURE_REASON", patient.reasonForRequestedProcedure);

                        if(strlen(patient.referencedSOPClassUID)>0)
                           _myVisit->set("TRAIT_SCHD_REF_STUDY_SOP_CLASS_UID", patient.referencedSOPClassUID);

                        if(strlen(patient.referencedSOPInstanceUID)>0)
                           _myVisit->set("TRAIT_SCHD_REF_STUDY_SOP_INSTANCE_UID", patient.referencedSOPInstanceUID);

                        visit->length(count+1);
                        (*visit)[count++] = _myVisit->getScheduledVisit();
                }
		catch (...) {
		        ::Message( MALARM, toEndUser | toService | MLoverall,
  	                           "Catch error when populating scheduledVisitList." );
		        throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
		}
        }
}

int WorklistManager::protocolName2Atlantis(char* worklist_key, char * buffer, int len)
{ 
  const char *pc;

  MutexGuard guard (_lockProtocolMapList);
  memset(buffer, 0, len);

  for(int i=0; i<(int)_myProtocolMapList.length(); i++) {
     for(int j=0; j<(int)_myProtocolMapList[i].rules.length(); j++) {
        pc = _myMatchUtils->getString(_myProtocolMapList[i].rules[j]);
        if( pc==NULL ) {
                Message( MWARNING, MLoverall | toService | toDeveloper,
                         "Unable to extract the _myProtocolMapList[%d].rules[%d].", i, j);
                continue;
        }
        else {
                if(strcmp(worklist_key, pc))
                        continue;
                _ptrToTraits->getTraitValue( _myProtocolMapList[i].subject, buffer, len);
                guard.release();
                return 1;
        }
     }
  }

  guard.release();
  return 0;
}

int WorklistManager::physicianName2Atlantis(char* name)
{
  char lastname[64], firstname[64];
  char *p;

  // Check for '=' the component group delimiter.
  // And only use the first component group.
  p = strchr(name, '=');
  if (p!=NULL)
	*p = '\0';

  strcpy(lastname, name);
  p = strchr(lastname, '^');
  if (p==NULL) {
      // no conversion needed
      return 1;
  }
  *p = '\0';

  p++;
  while (*p == '^') p++;
  strcpy(firstname, p);

  p = strchr(firstname, '^');
  if (p!=NULL)
      *p = '\0';

  if(strlen(firstname) > 0 && strlen(lastname) > 0) {
    strcpy(name, lastname);
    strcat(name, ", ");
    strcat(name, firstname);
  }
  else {
    // either one or both names are '\0', hence no ", " seperation is needed
    strcpy(name, lastname);
    strcat(name, firstname);
  }
  return 1;
}

int WorklistManager::patientName2Atlantis(char* name, char* lastname, char* firstname)
{
  char *p;

  // Check for '=' the component group delimiter.
  // And only use the first component group.
  p = strchr(name, '=');
  if (p!=NULL)
	*p = '\0';

  strcpy(lastname, name);
  p = strchr(lastname, '^');
  if (p==NULL) {
      firstname[0] = '\0';
      return 1;
  }
  *p = '\0';

  p++;
  while (*p == '^') p++;
  strcpy(firstname, p);

  p = strchr(firstname, '^');
  if (p!=NULL)
      *p = '\0';

  return 1;
}

void WorklistManager::date2Atlantis(char* date)
{	char buf1[DA_LEN+1], buf2[DA_LEN+1], *tmp;
	int i, len;
	long n;

	/*
	** Handle the old date format YYYY.MM.DD and convert it to be YYYYMMDD for StartDate
	*/
	memset(buf2, 0, DA_LEN+1);
	tmp = buf2;

	strcpy(buf1, date);
	len = strlen(buf1);
	for (i=0; i<len; i++)
	{	if(isdigit(buf1[i]))
			*tmp++ = buf1[i];
	}
	*tmp = '\0';

	if( strlen(buf2)==8 )
	{	// Check for MMDDYYYY
		n = atol(buf2);
		if( n<13000000 )
		{ // Cannot be YYYYMMDD, must be MMDDYYYY
			strcpy(buf1, buf2);
			buf1[4]='\0';
			sprintf(buf2, "%s%s", buf2+4, buf1);
		}
	}

	strcpy(date, buf2);
}

void WorklistManager::patientRec2Atlantis(WorkPatientRec* patient)
{   float pheight = 0;
	char tm_buf1[TM_LEN+1], tm_buf2[TM_LEN+1], *tmp;
	int i, len;

	/*
	** First convert the patient height from meter to cm
	*/
	pheight = atof(patient->patientsHeight);
	if( pheight < 0.0000000001 )
		patient->patientsHeight[0]='\0';

	else if( pheight < 3.0 ) // pheight is in meter and needs to be converted into cm
	{	pheight *= 100.0;
		sprintf(patient->patientsHeight, "%f", pheight);
	}

	/*
	** Second handle the StartDate
	*/
	date2Atlantis(patient->scheduledProcedureStepStartDate);

	/*
	** Third handle the old time format HH:MM:SS and convert it to be HHMMSS
	*/
	memset(tm_buf2, 0, TM_LEN+1);
	tmp = tm_buf2;

	strcpy(tm_buf1, patient->scheduledProcedureStepStartTime);
	len = strlen(tm_buf1);
	for (i=0; i<len; i++)
	{	if(isdigit(tm_buf1[i]))
			*tmp++ = tm_buf1[i];
	}
	*tmp = '\0';
	strcpy(patient->scheduledProcedureStepStartTime, tm_buf2);

	/*
	** Fourth handle the DOB
	*/
	date2Atlantis(patient->patientsDOB);
}
