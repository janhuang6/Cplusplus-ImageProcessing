#ifndef _WORKLISTMANAGER_H_
#define _WORKLISTMANAGER_H_

/*
 * file:	worklistmanager.h
 * purpose:	WorklistManager class
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */

#include "control/traitutils.h"
#include "control/lookupmatchutils.h"
#include "control/visitutils.h"
#include "control/connectfacility.h"
#include "control/connectfacilitysession.h"
#include "worklist/worklistutils.h"

#include "acquire/server.h"
#include "acquire/mesg.h"
#include "acquire/threadmutex.h"

static const char componentName[] = "worklistserver";

typedef DictionaryPkg::QueryType QueryType;
typedef DictionaryPkg::QueryStatus QueryStatus;

class WorklistManager : public MesgProducer {
	ThreadMutex _lockProtocolMapList; /* Serialize access to protocolMapList.*/

	ConnectFacilitySession *_session;
	char _sysLocalAETitle[64];
	static int _applicationID;/* One per app, after registered    */
	int _associationID;       /* ID of the association of this    */
                                  /* app, and the server.             */
	LIST _patientList;        /* The list of patients to process  */
	VisitUtils *_myVisit;
	WorklistUtils *_ptrToWorklistUtils;
                                 /* WorkList utils                   */
	DictionaryPkg::TraitAttrList * _dictionary;
	DICOMPkg::ProtocolMapList _myProtocolMapList;

	virtual QueryType getQueryType() = 0;
	void getQueryDates(char* dates);
	void processQueryParameter(QueryPara &qpar);
	void processReturnListInfo(PatientPkg::ScheduledVisitList *visit);
	int protocolName2Atlantis(char* worklist_key, char * buffer, int len);
	int physicianName2Atlantis(char* name);
	int patientName2Atlantis(char* name, char* lastname, char* firstname);
	void date2Atlantis(char* date);
	void patientRec2Atlantis(WorkPatientRec* patient);

	// Disallow copying or assignment.
	WorklistManager(const WorklistManager&);
	WorklistManager& operator=(const WorklistManager&);

#ifdef MYDEBUG
	void printTraitList(const DictionaryPkg::TraitValueList &traits);
#endif

protected:
	ThreadMutex _lockLookupMatchList; /* Serialize access to lookupMatchList.*/

	ConnectFacility *_connection;
	long _queryID;
	int _numberOfDaysToQuery; /* Query patient info for the number of days starting today */
	DictionaryPkg::LookupMatchList _myLookupMatchList;
                                  /* My LookupMatchList               */
	TraitUtils *_ptrToTraits; /* TraitUtils of Atlantis           */
	LookupMatchUtils *_myMatchUtils;

	static void* _sendWorklistQuery(void* thisClass);

public:
	WorklistManager( const char *mergeIniFile);
	virtual ~WorklistManager() throw();

	void setProtocolMapping(const DICOMPkg::ProtocolMapList& mapping);
};

#endif
