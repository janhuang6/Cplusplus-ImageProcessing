#ifndef _WORKLISTUTILS_H_
#define _WORKLISTUTILS_H_

/*
 * file:	worklistutils.h
 * purpose:	WorklistUtils method class
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */

/*
** We define INPUT_LEN to be 256.  This is the number of characters that the
** user can type before we overflow.
*/
const int INPUT_LEN = 256;

#include <string.h>

#include "mergecom.h" 
#include "mc3msg.h"
#include "mcstatus.h"
#include "diction.h"
#include "qr.h"

#include "acquire/mesg.h"

#ifndef MYDEBUG
//#define MYDEBUG 1
#endif

/*
** This method is moved from class WorklistUtils here
** because this method should only be called once per
** application. Not per class instantiation. Otherwise,
** get error MC_LIBRARY_ALREADY_INITIALIZED
*/
int PerformMergeInitialization( const char *mergeIniFile, int *p_applicationID, const char *p_localAppTitle );

struct WorkPatientRec
{
	// Scheduled Procedure Step Sequence (0040,0100)
	char scheduledStationLocalAETitle[ AE_LEN+1 ];       // >(0040,0001)
	char scheduledProcedureStepStartDate[ DA_LEN+1 ];    // >(0040,0002)
	char scheduledProcedureStepStartTime[ TM_LEN+1 ];    // >(0040,0003)
	char modality[ CS_LEN+1 ];                           // >(0008,0060)
	char scheduledPerformingPhysiciansName[ PN_LEN+1 ];  // >(0040,0006)
	char scheduledProcedureStepDescription[ LO_LEN+1 ];  // >(0040,0007)
	char scheduledStationName[ SH_LEN+1 ];               // >(0040,0010)
	char scheduledProcedureStepID[ SH_LEN+1 ];           // >(0040,0009)

	// >Scheduled Protocol Code Sequence >(0040,0008)
	char scheduledProtocolCodeValue[ SH_LEN+1 ];         // >>(0008,0100)
	char scheduledProtocolCodingSchemeDesignator[ SH_LEN+1 ];// >>(0008,0102)
	char scheduledProtocolCodeMeaning[ LO_LEN+1 ];       // >>(0008,0104)
	// >Referenced Study Component Sequence >(0008,1111)
//	char referencedComponentSOPClassUID[ UI_LEN+1 ];     // >>(0008,1150)
//	char referencedComponentSOPInstanceUID[ UI_LEN+1 ];  // >>(0008,1155)

	char requestedProcedureID[ SH_LEN+1 ];               // (0040,1001)
	char requestedProcedureDescription[ LO_LEN+1 ];      // (0032,1060)
	// Requested Procedure Code Sequence (0032,1064)
	char requestedProcedureCodeMeaning[ LO_LEN+1 ];      // (0008,0104)
	char studyInstanceUID[ UI_LEN+1 ];                   // (0020,000D)
	char accessionNumber[ SH_LEN+1 ];                    // (0008,0050)
	char requestingPhysician[ PN_LEN+1 ];                // (0032,1032)
	char referringPhysician[ PN_LEN+1 ];                 // (0008,0090)
	char patientsName[ PN_LEN+1 ];                       // (0010,0010)
	char patientID[ LO_LEN+1 ];                          // (0010,0020)
	char patientsDOB[ DA_LEN+1 ];                        // (0010,0030)
	char patientsHeight[ DS_LEN+1 ];                     // (0010,1020)
	char patientsWeight[ DS_LEN+1 ];                     // (0010,1030)
	char patientsGender[ CS_LEN+1 ];                     // (0010,0040)
	char patientState[ LO_LEN+1 ];                       // (0038,0500)
	char pregnancyStatus[ LO_LEN+1 ];                    // (0010,21C0)
	char medicalAlerts[ LO_LEN+1 ];                      // (0010,2000)
	char contrastAllergies[ LO_LEN+1 ];                  // (0010,2110)
	char specialNeeds[ LO_LEN+1 ];                       // (0038,0050)
	char otherPatientID[ LO_LEN+1 ];                     // (0010,1000)
	char intendedRecipients[ PN_LEN+1 ];                 // (0040,1010)
	char imageServiceRequestComments[ INPUT_LEN ];       // (0040,2400) LT_LEN
	char requestedProcedureComments[ INPUT_LEN ];        // (0040,1400) LT_LEN
	// Referenced Series Sequence (0008,1115)
	char seriesInstanceUID[ UI_LEN+1 ];                  // (0020,000E)
	char reasonForRequestedProcedure[ LO_LEN+1 ];        // (0040,1002)

	// Referenced Study Sequence (0008,1110)
	char referencedSOPClassUID[ UI_LEN+1 ];              // >(0008,1150)
	char referencedSOPInstanceUID[ UI_LEN+1 ];           // >(0008,1155)
};

enum { scheduledStationLocalAETitle,       // (0040,0001)
	   scheduledProcedureStepStartDate,    // (0040,0002)
	   scheduledProcedureStepStartTime,    // (0040,0003)
	   modality,                           // (0008,0060)
	   scheduledPerformingPhysiciansName,  // (0040,0006)
	   scheduledProcedureStepDescription,  // (0040,0007)
	   scheduledStationName,               // (0040,0010)
	   scheduledProcedureStepID,           // (0040,0009)
	   scheduledProtocolCodeValue,       // >>(0008,0100)
	   scheduledProtocolCodingSchemeDesignator, // >>(0008,0102)
	   scheduledProtocolCodeMeaning,       // (0008,0104)
//	   referencedComponentSOPClassUID,     // >>(0008,1150)
//	   referencedComponentSOPInstanceUID,  // >>(0008,1155)
	   requestedProcedureID,               // (0040,1001)
	   requestedProcedureDescription,      // (0032,1060)
	   requestedProcedureCodeMeaning,      // (0008,0104)
	   studyInstanceUID,                   // (0020,000D)
	   accessionNumber,                    // (0008,0050)
	   requestingPhysician,                // (0032,1032)
	   referringPhysician,                 // (0008,0090)
	   patientsName,                       // (0010,0010)
	   patientID,                          // (0010,0020)
	   patientsDOB,                        // (0010,0030)
	   patientsHeight,                     // (0010,1020)
	   patientsWeight,                     // (0010,1030)
	   patientsGender,                     // (0010,0040)
	   patientState,                       // (0038,0500)
	   pregnancyStatus,                    // (0010,21C0)
	   medicalAlerts,                      // (0010,2000)
	   contrastAllergies,                  // (0010,2110)
	   specialNeeds,                       // (0038,0050)
	   otherPatientID,                     // (0010,1000)
	   intendedRecipients,                 // (0040,1010)
	   imageServiceRequestComments,        // (0040,2400)
	   requestedProcedureComments,         // (0040,1400)
	   seriesInstanceUID,                  // (0020,000E)
	   reasonForRequestedProcedure,        // (0040,1002)
	   referencedSOPClassUID,              // >(0008,1150)
	   referencedSOPInstanceUID            // >(0008,1155)
};

struct QueryPara
{
    char localAeTitle[ AE_LEN+1 ];
    char scheduledProcStepStartDate[ DA_LEN*2 ];       /* YYYYMMDD-YYYYMMDD */
    char scheduledProcStepStartTime[ TM_LEN+1 ];
    char modality[ CS_LEN+1 ];
    char scheduledPerformPhysicianName[ PN_LEN+1 ];
    char accessionNumber[ SH_LEN+1 ];
    char requestingPhysician[ PN_LEN+1 ];
    char referringPhysician[ PN_LEN+1 ];
    char patientName[ PN_LEN+1 ];
    char patientID[ LO_LEN+1 ];
};

class WorklistUtils : public MesgProducer
{	/* Private variables */
	char _destAppTitle[ AE_LEN+1 ];
	int  _portNumber;
	char _remoteHost[ AE_LEN+1 ];
	int  _timeout;
	int  _threshold;

	char _attributeBlocked[38];

	void getAttributeFilter();

	int setNullOrStringValue( int p_messageID,
                              unsigned long p_tag,
                              const char* p_value,
                              const char* p_function );
	int setStringValue( int p_messageID,
                        unsigned long p_tag,
                        const char* p_value,
                        const char* p_function );

	// Disallow copying or assignment.
	WorklistUtils(const WorklistUtils&);
	WorklistUtils& operator=(const WorklistUtils&);

public:

	enum WorklistUtilsException {
	    WORKLISTUTILS_MEMORY,
	    WORKLISTUTILS_BADARG,
	    WORKLISTUTILS_SOFTWARE,
	    WORKLISTUTILS_NETWORK,
	    WORKLISTUTILS_UNKNOWN
	};

	WorklistUtils(const char* p_destAppTitle,
                      int p_portNumber,
                      const char* p_remoteHost,
                      int p_timeout,
                      int p_threshold
                      );
	~WorklistUtils() throw ();

	void setDestAppTitle(char* p_destAppTitle);
	void setPortNumber(int p_portNumber);
	void setRemoteHost(char* p_remoteHost);
	void setTimeout(int p_timeout);
	void setThreshold(int p_threshold);

	int openAssociation( int p_applicationID )
                             throw (WorklistUtilsException);
	void sendWorklistQuery( int p_associationID,
                                QueryPara queryPara )
                                throw (WorklistUtilsException);
	void processWorklistReplyMsg( int p_associationID,
                                      LIST *p_list )
                                      throw (WorklistUtilsException);
	void sendCcancelRQ( int p_associationID )
                            throw (WorklistUtilsException);
};

inline void
WorklistUtils::setDestAppTitle(char* p_destAppTitle)
{
	strcpy(_destAppTitle, p_destAppTitle);
}

inline void
WorklistUtils::setPortNumber(int p_portNumber)
{
	_portNumber = p_portNumber;
}

inline void
WorklistUtils::setRemoteHost(char* p_remoteHost)
{
	strcpy(_remoteHost, p_remoteHost);
}

inline void
WorklistUtils::setTimeout(int p_timeout)
{
	_timeout = p_timeout;
}

inline void
WorklistUtils::setThreshold(int p_threshold)
{
	_threshold = p_threshold;
}

#endif
