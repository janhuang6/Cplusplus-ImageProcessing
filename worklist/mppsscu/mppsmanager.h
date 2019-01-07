#ifndef _MPPSMANAGER_H_
#define _MPPSMANAGER_H_

//#define MYDEBUG 1

/*
 * file:	mppsmanager.h
 * purpose:	MppsManager class
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */

#include "control/traitutils.h"
#include "control/lookupmatchutils.h"
#include "control/visitutils.h"
#include "control/connectfacility.h"
#include "control/connectfacilitysession.h"
#include "mpps/mppsutils.h"

#include "acquire/server.h"
#include "acquire/mesg.h"

static const char componentName[] = "mppsscu";

class MppsManager : public MesgProducer {
	ConnectFacilitySession *_session;
	ConnectFacility *_connection;

	char _sysLocalAETitle[65];
	static int _applicationID;/* One per app, after registered    */

	VisitUtils *_myVisit;
	TraitUtils *_ptrToTraits; /* TraitUtils of Atlantis */
	MppsUtils *_ptrToMppsUtils; /* Mpps utils                   */

	DictionaryPkg::TraitAttrList * _dictionary;

	DictionaryPkg::TraitValueList_var _facilityInfo;
	char _isMPPSLicensed[8];

	void getPatientRec(WorkPatientRec &patRec, const PatientPkg::ScheduledVisit& currentVisit);
	void physicianName2Dicom(char* name);
	void patientName2Dicom(const char* lastname, const char* firstname, char* name);
	void patientGender2Dicom(const char* genderString, char* dcm_gender);
//    void pregnancyStatus2Dicom(const char* pregnancyStatusString, unsigned int &pregnancyStatus);

	// Disallow copying or assignment.
	MppsManager(const MppsManager&);
	MppsManager& operator=(const MppsManager&);

#ifdef MYDEBUG
	void printTraitList(const DictionaryPkg::TraitValueList &traits);
#endif

public:
	MppsManager( const char *mergeIniFile);
	virtual ~MppsManager() throw();

	char* startMPPSImageAcquisition( const PatientPkg::ScheduledVisit& currentVisit ) throw (MppsUtilsException);
	char* completeMPPS( const PatientPkg::ScheduledVisit& currentVisit ) throw (MppsUtilsException);
	char* discontinueMPPS( const PatientPkg::ScheduledVisit& currentVisit ) throw (MppsUtilsException);
};

#endif
