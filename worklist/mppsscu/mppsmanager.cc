/*
 * file:	mppsmanager.cc
 * purpose:	Implementation of the MppsManager class
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */

#include <unistd.h>
#include <stdlib.h>
#include <thread.h>
#include <ctype.h>
#include <iostream.h>

#include "mpps/mppsexceptions.h"
#include "mppsmanager.h"
#include "control/lookupmatchutils.h"
#include "control/stationimpl.h"

static const char rcsID[] = "$Id: mppsmanager.cc,v 1.27 jhuang Exp $";
static const char rcsName[] = "$Name: Atlantis710R01_branch $";
static const char userPW[] = "empty";
extern const char *mppsBuildDate;

int MppsManager::_applicationID = -1; /* initialize the static member, one per app */

#ifdef MYDEBUG
void 
MppsManager::printTraitList(const DictionaryPkg::TraitValueList &traits)
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

MppsManager::MppsManager(const char *mergeIniFile) :
   MesgProducer( toDeveloper | MLdatabase )
{
  char hostname[256];
  int iSleepCount=0;
  int timeout;           /* Timeout value                    */
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
  char MppsProviderDefined = 0;
  DictionaryPkg::TraitValueList_var facilityInfo = NULL;
  char isMPPSLicensed[8] ;

  _myVisit = NULL;
  _ptrToMppsUtils = NULL;
  _ptrToTraits = NULL;

  _session = NULL;
  _dictionary = NULL;
  _connection = NULL;

// These lines of code are modified from testConnectFacilitySession.cc
  try {
    Message( MNOTE, "Mpps is attempting to connect to Facility.");
    gethostname(hostname,sizeof(hostname));
    Message( MNOTE, "Setting hostname as %s", hostname);
    //ConnectFacility::useName("ADAC01");
    //ConnectFacility::setUserType(DictionaryPkg::USER_INTERNAL);
    _connection = ConnectFacility::instance();
  } catch (... ) {
    const char *emsg = "Failed to connect to Facility.";
    Message( MALARM, MLproblem, emsg );
    throw MppsNoFacilityException( emsg );
  }

  Message( MNOTE, "Waiting for connection --");
  while (0==_connection->isReady()){ 
    sleep(2);
    iSleepCount +=2;
    Message( MNOTE, "Still waiting for connection %2d/60s . . .",iSleepCount);
    if (iSleepCount > 60){
      const char *emsg = "Facility connection timeout.";
      Message( MWARNING, MLproblem, emsg );
      throw MppsNoFacilityException(emsg);
    }
  }

  try {
    Message( MNOTE, "Creating StationImpl");
    station = new StationImpl( DICOMPkg::PerformedProcedureStep::COMPONENT_NAME, userPW, componentName, rcsName, mppsBuildDate );
  } catch (... ) {
      const char *emsg = "Failed to create a Station";
      Message( MWARNING, MLoverall | toService | toDeveloper, emsg );
      throw MppsException( emsg );
  }

  try {
    Message( MNOTE, "Attempting to get a Facility Session");
    _session = _connection->getSession( station, DictionaryPkg::USER_INTERNAL );
// The following line trying to fix the memory leak, caused core dump.
//	if(station) { delete station; station = NULL; }
  } catch (... ) {
      const char *emsg = "Failed to get a Facility Session";
      Message( MWARNING, MLproblem, emsg );
      throw MppsException( emsg );
  }
  Message( MNOTE, "Successful in getting Facility Session");

  try {
        Message( MNOTE, "Attempting to get Trait Dictionary");
        _dictionary = _connection->getTraits();
  } catch (... ) {
    const char *emsg = "Failed to get Trait Dictionary";
    Message( MWARNING, MLproblem, emsg );
    throw MppsException( emsg );
  }

  if(_dictionary != NULL && _dictionary->length() != 0)
     Message( MNOTE, "Successful in getting Trait Dictionary");
  else {
     const char *emsg = "Got null pointer for the Trait Dictionary or the length is zero";
     Message( MWARNING, MLproblem, emsg );
     throw MppsException( emsg );
  }

  try {
        Message( MNOTE, "Creating an instance of TraitUtils");
        _ptrToTraits = new TraitUtils(_dictionary);
  } catch (... ) {
    const char *emsg = "Failed to instantiate TraitUtils";
    Message( MWARNING, MLproblem, emsg );
    throw MppsException( emsg );
  }
  Message( MNOTE, "Successful in instantiating a TraitUtils");

  facilityInfo = _connection->getFacilityInfo();
  memset(isMPPSLicensed,0,sizeof(isMPPSLicensed));
  _ptrToTraits->getTraitValue( "TRAIT_FACL_IS_MPPS_LICENSED", facilityInfo, isMPPSLicensed, sizeof(isMPPSLicensed));
  if (0!=strcasecmp(isMPPSLicensed,"true")){
    const char *emsg = "MPPS is not licensed. Exit";
    Message( MWARNING, MLproblem, emsg );
    sleep(5);  // hang around just long enough that the startup scripts see we have started
    throw MppsNotLicensedException(emsg);
  }

  Message( MNOTE, "Successfully getting the MPPS License");

  try {
        Message( MNOTE, "Creating an instance of VisitUtils");
        _myVisit = new VisitUtils(*_ptrToTraits);
  } catch (... ) {
        const char *emsg = "Failed to instantiate VisitUtils";
        Message( MWARNING, MLproblem, emsg );
        throw MppsException( emsg );
  }
  Message( MNOTE, "Successful in instantiating a VisitUtils");

// now get the SystemInfoList

  if(( systems = _connection->getSystemsInfo()) == NULL)
     {
       const char *emsg = "Cannot access sytem information from data server.";
       Message( MWARNING, MLproblem, emsg );
       throw MppsException( emsg );
     }

// search through the systems list for the requested save location

  for (int i=0; i< (int)systems->length(); i++)
     {
       curSystem = &(*systems)[i];
       services = &(curSystem->services);
       if (services->length() == 0)
           continue;
       strllcpy(systemName, curSystem->id.name, sizeof(systemName));
       Message( MNOTE, "Check system %s with %d services", systemName, services->length() );
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
            if (strcmp(serviceType, "MppsProvider") != 0)
                continue;

			MppsProviderDefined = 1;
            try {
                  serviceProtocolValue =  _ptrToTraits->getValue("TRAIT_APPS_SERVICE_PROTOCOL", *serviceTraits);
            }
            catch (...) {
                  const char *emsg = "No protocol defined for a Mpps Provider service!";
                  Message( MWARNING, MLproblem, emsg );
                  throw MppsNotConfiguredException( emsg );
            }
            serviceProtocolValue.tval >>= serviceProtocol;

            serviceConfTraits = &(*services)[j].svcConfig;

            try {
                  serviceHostValue = _ptrToTraits->getValue("TRAIT_APPS_CONF_HOST", *serviceConfTraits);
                  serviceHostValue.tval >>= serviceHost;
            }
            catch (...) {
                  Message( MWARNING, MLoverall | toService | toDeveloper, "No host defined for Mpps Provider service! Will use system name instead");
                  serviceHost = systemName;
            }

            if (strcmp(serviceProtocol, "DICOM") == 0)
            {
            try {
                  servicePortValue = _ptrToTraits->getValue("TRAIT_APPS_CONF_PORT", *serviceConfTraits);
                  servicePortValue.tval >>= servicePort;
            }
            catch (...) {
                  const char *emsg = "No port number defined for DICOM Mpps Provider service! Program cannot continue.";
                  Message( MWARNING, MLproblem, emsg );
                  throw MppsNotConfiguredException( emsg );
            }

            try {
                  serviceCalledAEValue = _ptrToTraits->getValue("TRAIT_APPS_CONF_CALLED_AE", *serviceConfTraits);
                  serviceCalledAEValue.tval >>= serviceCalledAETitle;
            }
            catch (...) {
                  const char *emsg = "No Called AE Title defined for DICOM Mpps Provider service! Program cannot continue.";
                  Message( MWARNING, MLproblem, emsg );
                  throw MppsNotConfiguredException( emsg );
            }

			memset(_sysLocalAETitle, 0, 64);
            try {
                  serviceCallingAEValue = _ptrToTraits->getValue("TRAIT_APPS_CONF_CALLING_AE", *serviceConfTraits);
                  serviceCallingAEValue.tval >>= serviceCallingAETitle;
                  strcpy(_sysLocalAETitle, serviceCallingAETitle);
				  if(strlen(serviceCallingAETitle)==0) {
					  serviceCallingAETitle = "PMS_MPPS_SCU";
	                  strcpy(_sysLocalAETitle, "PMS_MPPS_SCU"); // This is used when acquisition doesn't sent mppsscu the callingAETitle
				  }
            }
            catch (...) {
                  Message( MWARNING, MLoverall | toService | toDeveloper, "No Calling AE Title defined for DICOM Mpps Provider service! Will use default \"PMS_MPPS_SCU\".");
                  serviceCallingAETitle = "PMS_MPPS_SCU"; // Use "" will cause PerformInitialization: Unable to register this application with the library
                  strcpy(_sysLocalAETitle, "PMS_MPPS_SCU"); // This is used when acquisition doesn't sent mppsscu the callingAETitle
            }

            Message( MNOTE, "Found DICOM system: %s", systemName );
            break;
         }
      }
  }

  if(MppsProviderDefined == 0) {
          const char *emsg =  "No DICOM Mpps Provider service defined! Program cannot continue.";
	  Message( MWARNING, MLproblem, emsg );
	  throw MppsNotConfiguredException( emsg );
  }

  if (_applicationID == -1) {
        if(mergeIniFile==NULL || strlen(mergeIniFile) == 0) {
            const char *emsg = "#Software error: Merge INI file is NULL.";
            Message( MNOTE, MLproblem, emsg );
            throw MppsNotConfiguredException( emsg );
        }

        if(MERGE_FAILURE == PerformMergeInitialization( mergeIniFile, &_applicationID, serviceCallingAETitle )) {
            const char *emsg = "#Software error: Cannot initialize the Merge Toolkit.";
            Message( MNOTE, MLproblem, emsg );
            throw MppsException( emsg );
        }
  }

  // default value
  timeout = 10000;

  if(serviceCalledAETitle != NULL && servicePort != NULL && serviceHost != NULL)
  {
    char str[256];
    snprintf(str, 255, "Get serviceCalledAETitle = %s, servicePort = %s, and serviceHost = %s from the Data Server", serviceCalledAETitle, servicePort, serviceHost);
    // Has to get everything in str before calling the Message. WHY????
    Message( MNOTE, str);
    _ptrToMppsUtils = new MppsUtils(serviceCalledAETitle,
                                    atoi(servicePort), serviceHost,
                                    timeout);
  }
  else 
  {
    std::string emsg( "Got invalid parameters from MppsProvider service and could not construct MppsUtils:" );
    if ( ! serviceCalledAETitle )
        emsg += " serviceCalledAETitle==NULL ";
    if ( ! servicePort )
        emsg += " servicePort==NULL ";
    if ( ! serviceHost )
        emsg += " serviceHost==NULL ";
    Message( MWARNING, MLproblem, emsg.c_str() );
    throw MppsNotConfiguredException( emsg );
  }
}

MppsManager::~MppsManager() throw()
{
    delete _myVisit;
    delete _ptrToMppsUtils;
    delete _ptrToTraits;

	delete _session;
	delete _dictionary;
// connectFacility only defines a private destructor:
//	delete _connection;

    /*
    ** The last thing that we do is to release this application from the
    ** library.
    */
    MC_Release_Application( &_applicationID );

    if (MC_Library_Release() != MC_NORMAL_COMPLETION)
		Message( MALARM, toEndUser | toService | MLoverall, "Error releasing the library.");
}

void MppsManager::getPatientRec(WorkPatientRec &patRec, const PatientPkg::ScheduledVisit& currentVisit)
{
	DictionaryPkg::TraitValue trait;
	char firstname[64], lastname[64];
	const char *pchar;
	static unsigned long count=0;
			
#ifdef MYDEBUG
// The next few lines are only for debug purpose
	printTraitList(currentVisit.personal);
	printTraitList(currentVisit.schedule);
	printTraitList(currentVisit.biometric);
	printTraitList(currentVisit.location);
#endif
	_myVisit->useScheduledVisit(currentVisit);
	patRec.clear();

// Currently the following modality trait is not in the list, will catch the exception
	try {
		trait = _myVisit->getValue( "TRAIT_ACQR_MODALITY" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.modality, pchar, sizeof(patRec.modality));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get the modality from the acquisition.");
	}
// Provide our default modality if blank, otherwise the RIS may reject the message
	if ( strlen(patRec.modality) <= 0 )
	{
		strcpy (patRec.modality, "NM");
        Message( MNOTE, MLoverall | toDeveloper,
                 "getPatientRec: The modality from the acquisition is blank. Use default \"NM\"" );
	}
#ifdef MYDEBUG
printf("modality=%s\n", patRec.modality);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_SCHD_PROCEDURE_STEP_ID" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.scheduledProcedureStepID, pchar, sizeof(patRec.scheduledProcedureStepID));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_SCHD_PROCEDURE_STEP_ID from the acquisition.");
	}
// By DICOM, scheduledProcedureStepID shall be empty.
#ifdef MYDEBUG
printf("scheduledProcedureStepID=%s\n", patRec.scheduledProcedureStepID);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_SCHD_PROCEDURE_STEP_DESCRIPTION" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.scheduledProcedureStepDescription, pchar, sizeof(patRec.scheduledProcedureStepDescription));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_SCHD_PROCEDURE_STEP_DESCRIPTION from the acquisition.");
	}
#ifdef MYDEBUG
printf("scheduledProcedureStepDescription=%s\n", patRec.scheduledProcedureStepDescription);
#endif

// Provide the AETitle, otherwise the RIS will reject the message
	strllcpy (patRec.performedStationAETitle, _sysLocalAETitle, sizeof(patRec.performedStationAETitle));
#ifdef MYDEBUG
printf("performedStationAETitle=%s\n", patRec.performedStationAETitle);
#endif

// Merge RIS is okay if the TRAIT_SCHD_STATION_NAME is blank
	try {
		trait = _myVisit->getValue( "TRAIT_SCHD_STATION_NAME" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.performedStationName, pchar, sizeof(patRec.performedStationName));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_SCHD_STATION_NAME from the acquisition.");
	}
#ifdef MYDEBUG
printf("performedStationName=%s\n", patRec.performedStationName);
#endif

// Merge RIS is okay if the MC_ATT_PERFORMED_LOCATION is blank
// TRAIT_CAMR_LOCATION is from camera information, not acquisition.
// So we do not have value for MC_ATT_PERFORMED_LOCATION
#ifdef MYDEBUG
printf("performedLocation=%s\n", patRec.performedLocation);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_MPPS_START_DATE" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.performedProcedureStepStartDate, pchar, sizeof(patRec.performedProcedureStepStartDate));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_MPPS_START_DATE from the acquisition.");
	}
// Provide the default TODAY if blank, otherwise the RIS may reject the message
	if ( strlen(patRec.performedProcedureStepStartDate) <= 0 )
	{
		time_t       timeT;
		struct tm*   timePtr;

		timeT = time(NULL);
		timePtr = localtime(&timeT);
		sprintf(patRec.performedProcedureStepStartDate, "%4d%2d%2d",
				(timePtr->tm_year + 1900),
				(timePtr->tm_mon + 1),
				timePtr->tm_mday); //YYYYMMDD
        Message( MNOTE, MLoverall | toDeveloper,
                 "getPatientRec: performedProcedureStepStartDate from the acquisition is blank. Use default TODAY %s",
				 patRec.performedProcedureStepStartDate );
	}
#ifdef MYDEBUG
printf("performedProcedureStepStartDate=%s\n", patRec.performedProcedureStepStartDate);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_MPPS_START_TIME" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.performedProcedureStepStartTime, pchar, sizeof(patRec.performedProcedureStepStartTime));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_MPPS_START_TIME from the acquisition.");
	}
// Provide the default system time if blank, otherwise the RIS may reject the message
	if ( strlen(patRec.performedProcedureStepStartTime) <= 0 )
	{
		time_t       timeT;
		struct tm*   timePtr;

		timeT = time(NULL);
		timePtr = localtime(&timeT);
		sprintf(patRec.performedProcedureStepStartTime, "%2d%2d%2d",
			    timePtr->tm_hour,
				timePtr->tm_min,
				timePtr->tm_sec); //HHMMSS
        Message( MNOTE, MLoverall | toDeveloper,
                 "getPatientRec: performedProcedureStepStartTime from the acquisition is blank. Use default system time %s",
				 patRec.performedProcedureStepStartTime );
	}
#ifdef MYDEBUG
printf("performedProcedureStepStartTime=%s\n", patRec.performedProcedureStepStartTime);
#endif

	// No TRAITs for performedProcedureStepEndDate and performedProcedureStepEndTime
	// will have to use system time at N_SET.

	try {
		trait = _myVisit->getValue( "TRAIT_MPPS_ID" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.performedProcedureStepID, pchar, sizeof(patRec.performedProcedureStepID));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_MPPS_ID from the acquisition.");
	}
// Provide performedProcedureStepID if blank, otherwise the RIS may reject the message
	if ( strlen(patRec.performedProcedureStepID) <= 0 )
	{
		memset (patRec.performedProcedureStepID, 0, sizeof(patRec.performedProcedureStepID));
		strllcpy (patRec.performedProcedureStepID, patRec.scheduledProcedureStepID, sizeof(patRec.performedProcedureStepID));

		if ( strlen(patRec.performedProcedureStepID) > 0 )
			Message( MWARNING, MLoverall | toDeveloper,
				     "MppsManager: The performedProcedureStepID from the acquisition is blank. Use scheduledProcedureStepID = \"%s\" for the performedProcedureStepID", 
					 patRec.performedProcedureStepID );
		else
		{	sprintf(patRec.performedProcedureStepID, "%ld", (long)time(0)+count++);

			Message( MWARNING, MLoverall | toDeveloper,
				     "MppsManager: The performedProcedureStepID and scheduledProcedureStepID from the acquisition are blank. Create a new performedProcedureStepID = \"%s\"", 
					 patRec.performedProcedureStepID );
		}
	}
#ifdef MYDEBUG
printf("performedProcedureStepID=%s\n", patRec.performedProcedureStepID);
#endif

// Merge RIS is okay if the TRAIT_MPPS_DESCRIPTION is blank
	try {
		trait = _myVisit->getValue( "TRAIT_MPPS_DESCRIPTION" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.performedProcedureStepDescription, pchar, sizeof(patRec.performedProcedureStepDescription));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_MPPS_DESCRIPTION from the acquisition.");
	}
#ifdef MYDEBUG
printf("performedProcedureStepDescription=%s\n", patRec.performedProcedureStepDescription);
#endif

	// No TRAIT for performedProcedureTypeDescription

	try {
		trait = _myVisit->getValue( "TRAIT_SCHD_ATTENDING_PHYSICIAN" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.performingPhysiciansName, pchar, sizeof(patRec.performingPhysiciansName));
		physicianName2Dicom(patRec.performingPhysiciansName);
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_SCHD_ATTENDING_PHYSICIAN from the acquisition.");
	}
#ifdef MYDEBUG
printf("performingPhysiciansName=%s\n", patRec.performingPhysiciansName);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_SCHD_REQUESTED_PROCEDURE_ID" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.requestedProcedureID, pchar, sizeof(patRec.requestedProcedureID));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_SCHD_REQUESTED_PROCEDURE_ID from the acquisition.");
	}
// By DICOM, requestedProcedureID shall be empty.
#ifdef MYDEBUG
printf("requestedProcedureID=%s\n", patRec.requestedProcedureID);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_SCHD_REQUESTED_PROCEDURE_DESCRIPTION" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.requestedProcedureDescription, pchar, sizeof(patRec.requestedProcedureDescription));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_SCHD_REQUESTED_PROCEDURE_DESCRIPTION from the acquisition.");
	}
#ifdef MYDEBUG
printf("requestedProcedureDescription=%s\n", patRec.requestedProcedureDescription);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_SCHD_STUDY_INSTANCE_UID" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.studyInstanceUID, pchar, sizeof(patRec.studyInstanceUID));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_SCHD_STUDY_INSTANCE_UID from the acquisition.");
	}
#ifdef MYDEBUG
printf("studyInstanceUID=%s\n", patRec.studyInstanceUID);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_SCHD_ACCESSION_NUMBER" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.accessionNumber, pchar, sizeof(patRec.accessionNumber));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_SCHD_ACCESSION_NUMBER from the acquisition.");
	}
#ifdef MYDEBUG
printf("accessionNumber=%s\n", patRec.accessionNumber);
#endif

	memset(firstname, 0, sizeof(firstname));
	try {
		trait = _myVisit->getValue( "TRAIT_PERS_GIVENNAME" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(firstname, pchar, sizeof(firstname));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_PERS_GIVENNAME from the acquisition.");
	}

	memset(lastname, 0, sizeof(lastname));
	try {
		trait = _myVisit->getValue( "TRAIT_PERS_SURNAME" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(lastname, pchar, sizeof(lastname));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_PERS_SURNAME from the acquisition.");
	}

	patientName2Dicom(lastname, firstname, patRec.patientsName);
#ifdef MYDEBUG
printf("patRec.patientsName=%s\n", patRec.patientsName);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_PERS_IDNUMBER" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.patientID, pchar, sizeof(patRec.patientID));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_PERS_IDNUMBER from the acquisition.");
	}
#ifdef MYDEBUG
printf("patientID=%s\n", patRec.patientID);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_PERS_BIRTHDAY" );
		pchar = NULL;
		trait.tval >>= pchar;
//		DICOM expects YYYYMMDD date format, only 8 char long
		strncpy(patRec.patientsDOB, pchar, 8);
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_PERS_BIRTHDAY from the acquisition.");
	}
#ifdef MYDEBUG
printf("patientsDOB=%s\n", patRec.patientsDOB);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_PERS_GENDER" );
		pchar = NULL;
		trait.tval >>= pchar;
		patientGender2Dicom(pchar, patRec.patientsGender);
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_PERS_GENDER from the acquisition.");
	}
#ifdef MYDEBUG
printf("patientsGender=%s\n", patRec.patientsGender);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_SCHD_STUDY_ID" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.studyID, pchar, sizeof(patRec.studyID));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_SCHD_STUDY_ID from the acquisition.");
	}
// Provide studyID if blank, DICOM recommends to use Requested Procedure ID if studyID is blank.
	if ( strlen(patRec.studyID) <= 0 )
	{
		strllcpy (patRec.studyID, patRec.requestedProcedureID, sizeof(patRec.studyID));
#ifdef MYDEBUG
printf("MppsManager: The studyID from the acquisition is blank. Use requestedProcedureID = \"%s\" for the studyID.\n", 
				 patRec.studyID );
#endif
        Message( MWARNING, MLoverall | toDeveloper,
			     "MppsManager: The studyID from the acquisition is blank. Use requestedProcedureID = \"%s\" for the studyID", 
				 patRec.studyID );
	}
#ifdef MYDEBUG
printf("studyID=%s\n", patRec.studyID);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_MPPS_UID" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.SOPInstanceUID, pchar, sizeof(patRec.SOPInstanceUID));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_MPPS_UID from the acquisition.");
	}
#ifdef MYDEBUG
printf("SOPInstanceUID=%s\n", patRec.SOPInstanceUID);
#endif

	try {
		strllcpy(patRec.specificCharacterSet, LatinAlphabetNo1, sizeof(patRec.specificCharacterSet));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not set specificCharacterSet.");
	}
#ifdef MYDEBUG
printf("specificCharacterSet=%s\n", patRec.specificCharacterSet);
#endif

	patRec.acqImageRefs.clear();
	DictionaryPkg::TraitValueList_var tvl = NULL;
	try {
		DictionaryPkg::AcqImageRefList& airl = _myVisit->getProcedureStepUIDs();
		if (airl.length() > 0) {
			AcqImageRef acqImageRef;

#ifdef MYDEBUG
printf("length=%d\n", airl.length());
#endif
			for (unsigned int i=0; i<airl.length(); i++) {
				tvl = new DictionaryPkg::TraitValueList;
				tvl->length(2);
				tvl[0] = airl[i].seriesUID;
				tvl[1] = airl[i].imageUIDs;

				memset(&acqImageRef, 0, sizeof(AcqImageRef));
				try {
  					_ptrToTraits->getTraitValue( "TRAIT_NSET_IMAGE_UIDS", tvl, acqImageRef.NSETImageUIDs, sizeof(acqImageRef.NSETImageUIDs));
				} catch (...) {
					Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_NSET_IMAGE_UIDS from the acquisition.");
				}
#ifdef MYDEBUG
printf("NSETImageUIDs=%s\n", acqImageRef.NSETImageUIDs);
#endif

				try {
		  			_ptrToTraits->getTraitValue( "TRAIT_NSET_SERIES_UID", tvl, acqImageRef.seriesInstanceUID, sizeof(acqImageRef.seriesInstanceUID));
				} catch (...) {
					Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_NSET_SERIES_UID from the acquisition.");
				}
#ifdef MYDEBUG
printf("seriesInstanceUID=%s\n", acqImageRef.seriesInstanceUID);
#endif
				patRec.acqImageRefs.push_back(acqImageRef);
			} // for
		} // if
		else
			Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get the UIDS: zero length AcqImageRefList.");
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get the UIDS from the acquisition.");
	}

	try {
		trait = _myVisit->getValue( "TRAIT_SCHD_PROTOCOL_CODE_MEANING" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.protocolCodeMeaning, pchar, sizeof(patRec.protocolCodeMeaning));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_SCHD_PROTOCOL_CODE_MEANING from the acquisition.");
	}
#ifdef MYDEBUG
printf("protocolCodeMeaning=%s\n", patRec.protocolCodeMeaning);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_SCHD_PROTOCOL_CODE_VALUE" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.protocolCodeValue, pchar, sizeof(patRec.protocolCodeValue));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_SCHD_PROTOCOL_CODE_VALUE from the acquisition.");
	}
#ifdef MYDEBUG
printf("protocolCodeValue=%s\n", patRec.protocolCodeValue);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_SCHD_PROTOCOL_CODING_SCHEME" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.protocolCodingScheme, pchar, sizeof(patRec.protocolCodingScheme));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_SCHD_PROTOCOL_CODING_SCHEME from the acquisition.");
	}
#ifdef MYDEBUG
printf("protocolCodingScheme=%s\n", patRec.protocolCodingScheme);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_SCHD_PROCEDURE_CODE_MEANING" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.procedureCodeMeaning, pchar, sizeof(patRec.procedureCodeMeaning));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_SCHD_PROCEDURE_CODE_MEANING from the acquisition.");
	}
#ifdef MYDEBUG
printf("procedureCodeMeaning=%s\n", patRec.procedureCodeMeaning);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_SCHD_PROCEDURE_CODE_VALUE" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.procedureCodeValue, pchar, sizeof(patRec.procedureCodeValue));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_SCHD_PROCEDURE_CODE_VALUE from the acquisition.");
	}
#ifdef MYDEBUG
printf("procedureCodeValue=%s\n", patRec.procedureCodeValue);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_SCHD_PROCEDURE_CODING_SCHEME" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.procedureCodingScheme, pchar, sizeof(patRec.procedureCodingScheme));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_SCHD_PROCEDURE_CODING_SCHEME from the acquisition.");
	}
#ifdef MYDEBUG
printf("procedureCodingScheme=%s\n", patRec.procedureCodingScheme);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_MPPS_CODE_MEANING" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.performedProtocolCodeMeaning, pchar, sizeof(patRec.performedProtocolCodeMeaning));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_MPPS_CODE_MEANING from the acquisition.");
	}
#ifdef MYDEBUG
printf("performedProtocolCodeMeaning=%s\n", patRec.performedProtocolCodeMeaning);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_MPPS_CODE_VALUE" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.performedProtocolCodeValue, pchar, sizeof(patRec.performedProtocolCodeValue));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_MPPS_CODE_VALUE from the acquisition.");
	}
#ifdef MYDEBUG
printf("performedProtocolCodeValue=%s\n", patRec.performedProtocolCodeValue);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_MPPS_CODING_SCHEME" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.performedProtocolCodingScheme, pchar, sizeof(patRec.performedProtocolCodingScheme));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_MPPS_CODING_SCHEME from the acquisition.");
	}
#ifdef MYDEBUG
printf("performedProtocolCodingScheme=%s\n", patRec.performedProtocolCodingScheme);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_SCHD_REF_STUDY_SOP_CLASS_UID" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.referencedSOPClassUID, pchar, sizeof(patRec.referencedSOPClassUID));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_SCHD_REF_STUDY_SOP_CLASS_UID from the acquisition.");
	}
#ifdef MYDEBUG
printf("referencedSOPClassUID=%s\n", patRec.referencedSOPClassUID);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_SCHD_REF_STUDY_SOP_INSTANCE_UID" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.referencedSOPInstanceUID, pchar, sizeof(patRec.referencedSOPInstanceUID));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_SCHD_REF_STUDY_SOP_INSTANCE_UID from the acquisition.");
	}
#ifdef MYDEBUG
printf("referencedSOPInstanceUID=%s\n", patRec.referencedSOPInstanceUID);
#endif

	try {
		trait = _myVisit->getValue( "TRAIT_PROT_NAME" );
		pchar = NULL;
		trait.tval >>= pchar;
		strllcpy(patRec.protocolName, pchar, sizeof(patRec.protocolName));
	} catch (...) {
	    Message( MNOTE, MLoverall | toService | toDeveloper, "MppsManager could not get TRAIT_PROT_NAME from the acquisition.");
	}
#ifdef MYDEBUG
printf("protocolName=%s\n", patRec.protocolName);
#endif
}

void MppsManager::physicianName2Dicom(char* name)
{
  char *p=NULL;

  while ((p = strstr(name, ", ")) != NULL)
  {	*p = '^';
	*(p+1) = '^';
	p = NULL;
  }

  while ((p = strstr(name, ",")) != NULL)
  {	*p = '^';
	p = NULL;
  }
}

void MppsManager::patientName2Dicom(const char* lastname, const char* firstname, char* name)
{
	name[0] = '\0';

	if (strlen(lastname)>0 || strlen(firstname)>0) {
		if (strlen(lastname)>0)
			strcpy(name, lastname);
		
		if (strlen(lastname)>0 && strlen(firstname)>0)
			strcat(name, "^");

		if (strlen(firstname)>0)
			strcat(name, firstname);
	}
}

void MppsManager::patientGender2Dicom(const char* genderString, char* dcm_gender)
{
	if(!strncasecmp(genderString, "m", 1))
		strcpy(dcm_gender, "M");
	else if(!strncasecmp(genderString, "f", 1))
		strcpy(dcm_gender, "F");
	else
		strcpy(dcm_gender, "O");
}

#ifdef NOT_USED
void MppsManager::pregnancyStatus2Dicom(const char* pregnancyStatusString, unsigned int &pregnancyStatus)
{
    /*
    ** 1: "not pregnant"
	** 2: "possibly pregnant"
	** 3: "definitely pregnant"
	** 4: "unknown"
    */
	
    const char* strPregStat[4] = {"not pregnant", "possibly pregnant", "definitely pregnant", "unknown"};

	if(!strncasecmp(pregnancyStatusString, strPregStat[0], strlen(strPregStat[0])))
		pregnancyStatus = 1;
	else if(!strncasecmp(pregnancyStatusString, strPregStat[1], strlen(strPregStat[1])))
		pregnancyStatus = 2;
	else if(!strncasecmp(pregnancyStatusString, strPregStat[2], strlen(strPregStat[2])))
		pregnancyStatus = 3;
	else
		pregnancyStatus = 4;
}
#endif

char* MppsManager::startMPPSImageAcquisition( const PatientPkg::ScheduledVisit& currentVisit )
	throw (MppsUtilsException)
{
	// First check the license is still valid
	_facilityInfo = NULL;
	_facilityInfo = _connection->getFacilityInfo();
	memset(_isMPPSLicensed,0,sizeof(_isMPPSLicensed));
	_ptrToTraits->getTraitValue( "TRAIT_FACL_IS_MPPS_LICENSED", _facilityInfo, _isMPPSLicensed, sizeof(_isMPPSLicensed));
	if (0!=strcasecmp(_isMPPSLicensed,"true")){
		Message( MWARNING, MLoverall | toService | toDeveloper, "MPPS license has been disabled since server started. Now block the MPPS message.");
		throw ( MPPSUTILS_LICENSE );
	}

	WORK_PATIENT_REC patientRec;

	getPatientRec(patientRec, currentVisit);
	return _ptrToMppsUtils->startImageAcquisition( &patientRec, _applicationID);
}

char* MppsManager::completeMPPS( const PatientPkg::ScheduledVisit& currentVisit )
	throw (MppsUtilsException)
{
	// First check the license is still valid
	_facilityInfo = NULL;
	_facilityInfo = _connection->getFacilityInfo();
	memset(_isMPPSLicensed,0,sizeof(_isMPPSLicensed));
	_ptrToTraits->getTraitValue( "TRAIT_FACL_IS_MPPS_LICENSED", _facilityInfo, _isMPPSLicensed, sizeof(_isMPPSLicensed));
	if (0!=strcasecmp(_isMPPSLicensed,"true")){
		Message( MWARNING, MLoverall | toService | toDeveloper, "MPPS license has been disabled since server started. Now block the MPPS message.");
		throw ( MPPSUTILS_LICENSE );
	}

	WORK_PATIENT_REC patientRec;

	getPatientRec(patientRec, currentVisit);
	return _ptrToMppsUtils->completeMPPS( &patientRec, _applicationID);
}

char* MppsManager::discontinueMPPS( const PatientPkg::ScheduledVisit& currentVisit )
	throw (MppsUtilsException)
{
	// First check the license is still valid
	_facilityInfo = NULL;
	_facilityInfo = _connection->getFacilityInfo();
	memset(_isMPPSLicensed,0,sizeof(_isMPPSLicensed));
	_ptrToTraits->getTraitValue( "TRAIT_FACL_IS_MPPS_LICENSED", _facilityInfo, _isMPPSLicensed, sizeof(_isMPPSLicensed));
	if (0!=strcasecmp(_isMPPSLicensed,"true")){
		Message( MWARNING, MLoverall | toService | toDeveloper, "MPPS license has been disabled since server started. Now block the MPPS message.");
		throw ( MPPSUTILS_LICENSE );
	}

	WORK_PATIENT_REC patientRec;

	getPatientRec(patientRec, currentVisit);
	return _ptrToMppsUtils->discontinueMPPS( &patientRec, _applicationID);
}
