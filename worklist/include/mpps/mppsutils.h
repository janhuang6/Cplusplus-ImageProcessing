#ifndef _MPPSUTILS_H_
#define _MPPSUTILS_H_

/*
 * file:	mppsutils.h
 * purpose:	MppsUtils class
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
#include "worklist/qr.h"

#include "acquire/mesg.h"

#include <iostream>
#include <list>
using namespace std;

enum MPPS_STATUS {MPPS_IN_PROGRESS, MPPS_DISCONTINUED, MPPS_COMPLETED};
extern const char* LatinAlphabetNo1;

/*
** This method is moved from class MppsUtils here
** because this method should only be called once per
** application. Not per class instantiation. Otherwise,
** get error MC_LIBRARY_ALREADY_INITIALIZED
*/
int PerformMergeInitialization( const char *mergeIniFile, int *p_applicationID, const char *p_localAppTitle );


typedef struct _ACQ_IMAGE_REF
{
   char NSETImageUIDs[ UI_LEN*64+1 ];					// UIDs for (0040,0340)>(0008,1140)>(0008,1155)
   char seriesInstanceUID[ UI_LEN+1 ];					// 0x0020,000E
} ACQ_IMAGE_REF, AcqImageRef;

typedef struct _WORK_PATIENT_REC
{
   char modality[ CS_LEN+1 ];                           // (0008,0060)
   char scheduledProcedureStepID[ SH_LEN+1 ];           // (0040,0009)
   char scheduledProcedureStepDescription[ LO_LEN+1 ];  // (0040,0007)
   char performedStationAETitle[ AE_LEN+1 ];            // (0040,0241)
   char performedStationName[ SH_LEN+1 ];               // (0040,0242)
   char performedLocation[ LO_LEN+1 ];                  // (0040,0243)
   char performedProcedureStepStartDate[ DA_LEN+1 ];    // (0040,0244) //YYYYMMDD
   char performedProcedureStepStartTime[ TM_LEN+1 ];    // (0040,0245) //HHMMSS
   char performedProcedureStepEndDate[ DA_LEN+1 ];      // (0040,0250)
   char performedProcedureStepEndTime[ TM_LEN+1 ];      // (0040,0251)
   char performedProcedureStepID[ SH_LEN+1 ];           // (0040,0253)
   char performedProcedureStepDescription[ LO_LEN+1 ];  // (0040,0254)
   char performedProcedureTypeDescription[ LO_LEN+1 ];  // (0040,0255)
   char retrieveAETitle[ AE_LEN+1 ];                    // (0008,0054)
   char seriesDescription[ LO_LEN+1 ];                  // (0008,103E)
   char performingPhysiciansName[ PN_LEN+1 ];           // (0008,1050)
   char operatorsName[ PN_LEN+1 ];                      // (0008,1070)
   char requestedProcedureID[ SH_LEN+1 ];               // (0040,1001)
   char requestedProcedureDescription[ LO_LEN+1 ];      // (0032,1060)
   char studyInstanceUID[ UI_LEN+1 ];                   // (0020,000D)
   char accessionNumber[ SH_LEN+1 ];                    // (0008,0050)
   char patientsName[ PN_LEN+1 ];                       // (0010,0010)
   char patientID[ LO_LEN+1 ];                          // (0010,0020)
   char patientsDOB[ DA_LEN+1 ];                        // (0010,0030)
   char patientsGender[ CS_LEN+1 ];                     // (0010,0040)
   char studyID[ LO_LEN+1 ];                            // (0020,0010)
   char SOPInstanceUID[ UI_LEN+1 ];                     // (0008,0018)
   char specificCharacterSet[ CS_LEN+1 ];               // (0008,0005)
   list<AcqImageRef> acqImageRefs;						// To support multiple Series Instance UID (0020,000E)

   char protocolCodeMeaning[ LO_LEN+1 ];				// (0040,0100)>PROTOCOL_CODE_SEQUENCE 0x0040,0008
   char protocolCodeValue[ SH_LEN+1 ];					// PROTOCOL_CODE_SEQUENCE 0x0040,0008
   char protocolCodingScheme[ SH_LEN+1 ];				// PROTOCOL_CODE_SEQUENCE 0x0040,0008
   char procedureCodeMeaning[ LO_LEN+1 ];				// PROCEDURE_CODE_SEQUENCE 0x0008,1032
   char procedureCodeValue[ SH_LEN+1 ];					// PROCEDURE_CODE_SEQUENCE 0x0008,1032
   char procedureCodingScheme[ SH_LEN+1 ];				// PROCEDURE_CODE_SEQUENCE 0x0008,1032
   char performedProtocolCodeMeaning[ LO_LEN+1 ];		// PERFORMED_PROTOCOL_CODE_SEQUENCE 0x0040,0260
   char performedProtocolCodeValue[ SH_LEN+1 ];			// PERFORMED_PROTOCOL_CODE_SEQUENCE 0x0040,0260
   char performedProtocolCodingScheme[ SH_LEN+1 ];		// PERFORMED_PROTOCOL_CODE_SEQUENCE 0x0040,0260
   char referencedSOPClassUID[ UI_LEN+1 ];				// (0008,1110)>(0008,1150)
   char referencedSOPInstanceUID[ UI_LEN+1 ];			// (0008,1110)>(0008,1155)
   char protocolName[ LO_LEN+1 ];						// (0018,1030)

   void clear();
} WORK_PATIENT_REC, WorkPatientRec;

enum MppsUtilsException {
    MPPSUTILS_MEMORY,
    MPPSUTILS_BADARG,
    MPPSUTILS_SOFTWARE,
    MPPSUTILS_LICENSE,
    MPPSUTILS_NETWORK,
    MPPSUTILS_NCREATE,
    MPPSUTILS_NSET,
    MPPSUTILS_UNKNOWN
};

class MppsUtils : public MesgProducer
{	/* Private variables */
	char _destAppTitle[ AE_LEN+1 ];
	int  _portNumber;
	char _remoteHost[ AE_LEN+1 ];
	int  _timeout;
	
    char _affectedSOPinstance[ UI_LEN+1 ];
    char _mppsStatus[ CS_LEN+1 ];

	int setNullOrStringValue( int p_messageID,
                              unsigned long p_tag,
                              const char* p_value,
                              const char* p_function );
	int setStringValue( int p_messageID,
                        unsigned long p_tag,
                        const char* p_value,
                        const char* p_function );
	int sendNCREATERQ( int p_applicationID,
                       WORK_PATIENT_REC *p_patient );
	int setNCREATERQ( int p_messageID,
                      WORK_PATIENT_REC *p_patient );
	int sendNSETRQStatus( int p_applicationID,
                            WORK_PATIENT_REC *p_patient,
							MPPS_STATUS p_status);
	int setNSETRQ( int   p_messageID,
                   WORK_PATIENT_REC *p_patient);

	// Disallow copying or assignment.
	MppsUtils(const MppsUtils&);
	MppsUtils& operator=(const MppsUtils&);

public:

	MppsUtils(const char* p_destAppTitle,
              int p_portNumber,
              const char* p_remoteHost,
              int p_timeout
              );
	~MppsUtils() throw ();

	void setDestAppTitle(char* p_destAppTitle);
	void setPortNumber(int p_portNumber);
	void setRemoteHost(char* p_remoteHost);
	void setTimeout(int p_timeout);

	int openAssociation( int p_applicationID )
                             throw (MppsUtilsException);
	char* startImageAcquisition( WORK_PATIENT_REC *p_patient,
                                int p_applicationID )
                             throw (MppsUtilsException);
	char* completeMPPS( WORK_PATIENT_REC *p_patient,
                       int p_applicationID )
                             throw (MppsUtilsException);
	char* discontinueMPPS( WORK_PATIENT_REC *p_patient,
                       int p_applicationID )
                             throw (MppsUtilsException);
};

inline void
MppsUtils::setDestAppTitle(char* p_destAppTitle)
{
	strcpy(_destAppTitle, p_destAppTitle);
}

inline void
MppsUtils::setPortNumber(int p_portNumber)
{
	_portNumber = p_portNumber;
}

inline void
MppsUtils::setRemoteHost(char* p_remoteHost)
{
	strcpy(_remoteHost, p_remoteHost);
}

inline void
MppsUtils::setTimeout(int p_timeout)
{
	_timeout = p_timeout;
}

// This is the same function as solaris strlcpy but
// defined for linux. We rename it so the code can
// also be compiled on solaris without dup definitions.
void strllcpy (char * dest, const char * src, size_t size);

#endif
