/*
 * file:	mppsutils.cc
 * purpose:	Implementation of MppsUtils class
 *
 * revision history:
 *   Jiantao Huang		    initial version
*/

#include <stdlib.h>
#include <ctype.h>

#include "mppsutils.h"

const char* ReferencedSOPClassUID = "1.2.840.10008.5.1.4.1.1.20";
const char* LatinAlphabetNo1 = "ISO_IR 100";

void _WORK_PATIENT_REC::clear()
{
	acqImageRefs.clear();

	memset(modality, 0, sizeof(modality));
	memset(scheduledProcedureStepID, 0, sizeof(scheduledProcedureStepID));
	memset(scheduledProcedureStepDescription, 0, sizeof(scheduledProcedureStepDescription));
	memset(performedStationAETitle, 0, sizeof(performedStationAETitle));
	memset(performedStationName, 0, sizeof(performedStationName));
	memset(performedLocation, 0, sizeof(performedLocation));
	memset(performedProcedureStepStartDate, 0, sizeof(performedProcedureStepStartDate));
	memset(performedProcedureStepStartTime, 0, sizeof(performedProcedureStepStartTime));
	memset(performedProcedureStepEndDate, 0, sizeof(performedProcedureStepEndDate));
	memset(performedProcedureStepEndTime, 0, sizeof(performedProcedureStepEndTime));
	memset(performedProcedureStepID, 0, sizeof(performedProcedureStepID));
	memset(performedProcedureStepDescription, 0, sizeof(performedProcedureStepDescription));
	memset(performedProcedureTypeDescription, 0, sizeof(performedProcedureTypeDescription));
	memset(retrieveAETitle, 0, sizeof(retrieveAETitle));
	memset(seriesDescription, 0, sizeof(seriesDescription));
	memset(performingPhysiciansName, 0, sizeof(performingPhysiciansName));
	memset(operatorsName, 0, sizeof(operatorsName));
	memset(requestedProcedureID, 0, sizeof(requestedProcedureID));
	memset(requestedProcedureDescription, 0, sizeof(requestedProcedureDescription));
	memset(studyInstanceUID, 0, sizeof(studyInstanceUID));
	memset(accessionNumber, 0, sizeof(accessionNumber));
	memset(patientsName, 0, sizeof(patientsName));
	memset(patientID, 0, sizeof(patientID));
	memset(patientsDOB, 0, sizeof(patientsDOB));
	memset(patientsGender, 0, sizeof(patientsGender));
	memset(studyID, 0, sizeof(studyID));
	memset(SOPInstanceUID, 0, sizeof(SOPInstanceUID));
	memset(specificCharacterSet, 0, sizeof(specificCharacterSet));
	memset(protocolCodeMeaning, 0, sizeof(protocolCodeMeaning));
	memset(protocolCodeValue, 0, sizeof(protocolCodeValue));
	memset(protocolCodingScheme, 0, sizeof(protocolCodingScheme));
	memset(procedureCodeMeaning, 0, sizeof(procedureCodeMeaning));
	memset(procedureCodeValue, 0, sizeof(procedureCodeValue));
	memset(procedureCodingScheme, 0, sizeof(procedureCodingScheme));
	memset(performedProtocolCodeMeaning, 0, sizeof(performedProtocolCodeMeaning));
	memset(performedProtocolCodeValue, 0, sizeof(performedProtocolCodeValue));
	memset(performedProtocolCodingScheme, 0, sizeof(performedProtocolCodingScheme));
	memset(referencedSOPClassUID, 0, sizeof(referencedSOPClassUID));
	memset(referencedSOPInstanceUID, 0, sizeof(referencedSOPInstanceUID));
	memset(protocolName, 0, sizeof(protocolName));
}

/*****************************************************************************
**
** NAME
**    PerformInitialization
**
** SYNOPSIS
**    Does the library initialization, registers this as an
**    application, and sets up some initial program values
**
**    This method is moved from class MppsUtils here
**    because this method should only be called once per
**    application. Not per class instantiation. Otherwise,
**    will get error MC_LIBRARY_ALREADY_INITIALIZED
**
*****************************************************************************/
int PerformMergeInitialization( const char *mergeIniFile, int *p_applicationID, const char *p_localAppTitle )
{
    MC_STATUS    status;

    /*
    ** This is to set the merge.ini file with full path.  We do it this way
    ** instead of using the environment variable MERGE_INI that might have
    ** been used by the Merge Java Toolkit. 
    */
    status = MC_Set_MergeINI(const_cast<char *>(mergeIniFile));
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "MC_Set_MergeINI failed: line %d status %d.", __LINE__, status);
        return( MERGE_FAILURE );
    }

    /*
    ** This is the initial call to the library.  It must be the first call
    ** to the toolkit that is made after MC_Set_MergeINI call
    */
    status = MC_Library_Initialization( NULL, NULL, NULL );
    if ( status == MC_LIBRARY_ALREADY_INITIALIZED )
    {
        Message( MWARNING, MLoverall | toDeveloper, "PerformInitialization: line %d status %d. The library is already initialized", __LINE__, status);
        return( MERGE_SUCCESS );
    }

    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "PerformInitialization: line %d status %d. Unable to initialize the Merge library", __LINE__, status);
        return( MERGE_FAILURE );
    }

    /*
    ** Once we've successfully initialized the library, then we must
    ** register this application with it.
    */
    status = MC_Register_Application( p_applicationID, p_localAppTitle );

    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "PerformInitialization: line %d status %d. Unable to register this application (calling AE Title = %s) with the library", __LINE__, status,  p_localAppTitle);
        return( MERGE_FAILURE );
    }

    return( MERGE_SUCCESS );
}

MppsUtils::MppsUtils(const char* p_destAppTitle,
                     int p_portNumber,
                     const char* p_remoteHost,
                     int p_timeout
                     )
{
	strcpy(_destAppTitle, p_destAppTitle);
	_portNumber = p_portNumber;
	strcpy(_remoteHost, p_remoteHost);
	_timeout = p_timeout;

	memset(_affectedSOPinstance, 0, sizeof(_affectedSOPinstance));
}

MppsUtils::~MppsUtils() throw ()
{
}

/*****************************************************************************
**
** NAME
**    openAssociation
**
** SYNOPSIS
**    int openAssociation
**
** ARGUMENTS
**    int  p_applicationID - The application ID 
**
** DESCRIPTION
**    Opens an association with the server based on the includance of,
**    or lack of, command line parameters.
**
** RETURNS
**    ID of the opened association
**
** SEE ALSO
**
*****************************************************************************/
int MppsUtils::openAssociation(int  p_applicationID) throw (MppsUtilsException)
{
    int          *attemptPort = (int *)NULL;
    char         *attemptHost = (char *)NULL;
    MC_STATUS    status;
    int          associationID;

    if ( _portNumber != 0 )
    {
        /*
        ** If we've been given a port number from the command line, then
        ** we want to make the attemptPort pointer point to this.  Hence,
        ** we make it point to the address of the _portNumber.
        */
        attemptPort = &_portNumber;
    }
    if ( _remoteHost[0] != '\0' )
    {
        /*
        ** If we've been given a command line argument, then we want to
        ** make the attemptHost pointer point to a character string
        ** that contains the host we've passed.
        */
        attemptHost = _remoteHost;
    }

    /*
    ** Now that we've gotten the command line arguments either copied or left
    ** NULL, we can attempt to open the association...
    */
    status = MC_Open_Association( p_applicationID, &associationID,
                                  _destAppTitle, attemptPort,
                                  attemptHost, const_cast<char *>("Worklist_Service_List") );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "openAssociation: line %d status %d. Unable to create an association with %s.", __LINE__, status, _destAppTitle );
        throw ( MPPSUTILS_NETWORK );
    }

    return associationID;
}


/*****************************************************************************
**
** NAME
**    setNullOrStringValue
**
** SYNOPSIS
**    int setNullOrStringValue
**
** ARGUMENTS
**    int               p_messageID - the id of a message
**    unsigned long     p_tag       - the tag to set
**    const char*       p_value     - the value to set
**    const char*       p_function  - a function name to reference
**
** DESCRIPTION
**    This function is used to set a Type 2 attribute within a message.
**    It takes a string as input.  If the string is zero length, it
**    sets the tag to NULL, if not, it sets the tag to the actual value.
**
** RETURNS
**    MERGE_SUCCESS or MERGE_FAILURE
**
** SEE ALSO
**
*****************************************************************************/
int
MppsUtils::setNullOrStringValue( 
                int               p_messageID,
                unsigned long     p_tag,
                const char*       p_value,
                const char*       p_function
            )
{
    MC_STATUS status;
    char      tagDescription[60];

    if ( strlen( p_value ) > 0 )
    {
        status = MC_Set_Value_From_String( p_messageID,
                                           p_tag,
                                           p_value );
        if ( status != MC_NORMAL_COMPLETION )
        {
            MC_Get_Tag_Info(p_tag, tagDescription, sizeof(tagDescription));
            Message( MWARNING, MLoverall | toDeveloper, "setNullOrStringValue: line %d, status %d. Failed to set the tag %s to \"%s\" for p_function \"%s\"", __LINE__, status, tagDescription, p_value, p_function );
        }
        else
            return ( MERGE_SUCCESS );
    }

    status = MC_Set_Value_To_NULL( p_messageID, p_tag );
    if ( status != MC_NORMAL_COMPLETION )
    {
        MC_Get_Tag_Info(p_tag, tagDescription, sizeof(tagDescription));
        Message( MWARNING, MLoverall | toDeveloper, "setNullOrStringValue: line %d, status %d. Failed to NULL \"%s\" for p_function \"%s\"", __LINE__, status, tagDescription, p_function );
        return ( MERGE_FAILURE );
    }
    
    return ( MERGE_SUCCESS );
}



/*****************************************************************************
**
** NAME
**    setStringValue
**
** SYNOPSIS
**    int setStringValue
**
** ARGUMENTS
**    int               p_messageID - the tag or message ID
**    unsigned long     p_tag       - the tag
**    const char*       p_value     - the value to set
**    const char*       p_function  - the calling function's name
**
** DESCRIPTION
**    This function is used to set string attributes within a message.
**    It takes a string as input and sets the value.  If a failure occurs,
**    it logs a warning message with the tag name.
**
** RETURNS
**    MERGE_SUCCESS or MERGE_FAILURE
**
** SEE ALSO
**    
*****************************************************************************/
int
MppsUtils::setStringValue( 
                int               p_messageID,
                unsigned long     p_tag,
                const char*       p_value,
                const char*       p_function
            )
{
    MC_STATUS status;
    char      tagDescription[60];

    status = MC_Set_Value_From_String( p_messageID,
                                       p_tag,
                                       p_value );
    if ( status != MC_NORMAL_COMPLETION )
    {
        MC_Get_Tag_Info(p_tag, tagDescription, sizeof(tagDescription));
        Message( MWARNING, MLoverall | toDeveloper, "setStringValue: line %d, status %d. Failed to set the tag %s to \"%s\" for p_function \"%s\"", __LINE__, status, tagDescription, p_value, p_function );
        return ( MERGE_FAILURE );
    }
    
    return ( MERGE_SUCCESS );
}



/*****************************************************************************
**
** NAME
**    startImageAcquisition
**
** SYNOPSIS
**    char* startImageAcquisition
**
** ARGUMENTS
**    WORK_PATIENT_REC *p_patient       - a pointer to a single patient rec
**    int               p_applicationID - the application ID
**
** DESCRIPTION
**    This function sends the initial N_CREATE message to the RIS/HIS,
**    and then gives the user the ability to do some modality imaging
**    functionality.
**
** RETURNS
**    affectedSOPinstance
**
** SEE ALSO
**    
*****************************************************************************/
char* MppsUtils::startImageAcquisition( WORK_PATIENT_REC *p_patient,
                                        int p_applicationID )
                             throw (MppsUtilsException)
{	/*
    ** We send out modality performed precedure step messages.
    */
    Message( MNOTE, MLoverall | toDeveloper, "Sending MPPS N-CREATE messages..." );
    if ( sendNCREATERQ( p_applicationID, 
                        p_patient ) != MERGE_SUCCESS )
    {
        Message( MWARNING, MLoverall | toDeveloper, "Failed to send an NCREATE request message to the RIS and receive a valid response back from the RIS." );
		throw (MPPSUTILS_NCREATE);
    }

	return _affectedSOPinstance;
}

/*****************************************************************************
**
** NAME
**    completeMPPS
**
** SYNOPSIS
**    char* completeMPPS
**
** ARGUMENTS
**    WORK_PATIENT_REC *p_patient       - a pointer to a single patient rec
**    int               p_applicationID - the application ID
**
** DESCRIPTION
**    This function sends the N_SET COMPLETED message to the RIS/HIS.
**
** RETURNS
**    MC_ATT_PERFORMED_PROCEDURE_STEP_STATUS
**
** SEE ALSO
**    
*****************************************************************************/
char* MppsUtils::completeMPPS( WORK_PATIENT_REC *p_patient,
                               int p_applicationID )
                              throw (MppsUtilsException)
{	/*
    ** The user chose to complete this procedure step.
    */
    Message( MNOTE, MLoverall | toDeveloper, "Sending N_SET RQ status=COMPLETED..." );
    if(sendNSETRQStatus( p_applicationID,
                         p_patient, MPPS_COMPLETED )==MERGE_FAILURE)
	{
		Message( MWARNING, MLoverall | toDeveloper, "Sending N_SET RQ status=COMPLETED throws exception." );
		throw (MPPSUTILS_NSET);
	}

	return _mppsStatus;
}

/*****************************************************************************
**
** NAME
**    discontinueMPPS
**
** SYNOPSIS
**    char* discontinueMPPS
**
** ARGUMENTS
**    WORK_PATIENT_REC *p_patient       - a pointer to a single patient rec
**    int               p_applicationID - the application ID
**
** DESCRIPTION
**    This function sends the N_SET DISCONTINUED message to the RIS/HIS.
**
** RETURNS
**    MC_ATT_PERFORMED_PROCEDURE_STEP_STATUS
**
** SEE ALSO
** 
*****************************************************************************/
char* MppsUtils::discontinueMPPS( WORK_PATIENT_REC *p_patient,
                                  int p_applicationID )
                             throw (MppsUtilsException)
{	/*
    ** The user chose to complete this procedure step.
    */
    Message( MNOTE, MLoverall | toDeveloper, "Sending N_SET RQ status=DISCONTINUED..." );
    if(sendNSETRQStatus( p_applicationID,
                         p_patient, MPPS_DISCONTINUED )==MERGE_FAILURE)
	{
		Message( MWARNING, MLoverall | toDeveloper, "Sending N_SET RQ status=DISCONTINUED throws exception." );
		throw (MPPSUTILS_NSET);
	}

	return _mppsStatus;
}

/*****************************************************************************
**
** NAME
**    sendNCREATERQ
**
** SYNOPSIS
**    int sendNCREATERQ(
**
** ARGUMENTS
**    int               p_applicationID       - the application ID
**    WORK_PATIENT_REC *p_patient             - a pointer to a single patient
**
** DESCRIPTION
**    This function sends a performed procedure step "IN PROGRESS"
**    to the worklist/mpps SCP.
**
** RETURNS
**    MERGE_SUCCESS or MERGE_FAILURE
**
** SEE ALSO
**    
*****************************************************************************/
int MppsUtils::sendNCREATERQ( int p_applicationID,
                       WORK_PATIENT_REC *p_patient )
{
    MC_STATUS    status;
    int          associationID;
    int          messageID;
    int          rspMsgID;
    unsigned int response;
    char         *rspSrvName;
    MC_COMMAND   rspCommand;

    /*
    ** In order to send the PERFORMED PROCEDURE STEP N_CREATE message to the
    ** SCP, we need to open a message.
    */
    status = MC_Open_Message( &messageID, "PERFORMED_PROCEDURE_STEP",
                              N_CREATE_RQ );
    if ( status != MC_NORMAL_COMPLETION )
    {
		Message( MWARNING, MLoverall | toDeveloper,
			"sendNCREATERQ: line %d status %d. Unable to open a message for PERFORMED_PROCEDURE_STEP",
			__LINE__, status);
        return ( MERGE_FAILURE );
    }

    /*
    ** Once we have an open message, we will then attempt to fill in the
    ** contents of the message.  This is done in a separate funtion.
    */
    if ( setNCREATERQ( messageID, p_patient ) == MERGE_FAILURE )
    {
        /*
        ** The error message was logged in the function, itself.  We still
        ** need to free the message.
        */
        Message( MWARNING, MLoverall | toDeveloper, "setNCREATERQ() returns MERGE_FAILURE at line=%d", __LINE__);
        MC_Free_Message( &messageID );
        return( MERGE_FAILURE );
    }

    /*
    ** Now that we've created our request message, we want to open the 
    ** association with an SCP that is providing MPPS.
    */
	try {
         associationID = openAssociation( p_applicationID );
	}
	catch (...)
    {
        MC_Free_Message( &messageID );
        return( MERGE_FAILURE );
    }
    
    
    /*
     * Validate and list the message
     */
    ValMessage      (messageID, "N-CREATE-RQ");
    WriteToListFile (messageID, "ccreaterq.msg");


    /*
    ** And finally, we send the N_CREATE message to the server.
    */
    status = MC_Send_Request_Message( associationID, messageID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper,
			"sendNCREATERQ: line %d status %d. Unable to send the N_CREATE message to the SCP.",
			__LINE__, status );
        MC_Abort_Association( &associationID );
        MC_Free_Message ( &messageID );
        return ( MERGE_FAILURE );
    }

    /*
    ** Free the request message, since we do not need to use it anymore.
    */
    status = MC_Free_Message( &messageID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "sendNCREATERQ: line %d status %d. Unable to free the N_CREATE message. Fatal error.",
                 __LINE__, status );
        MC_Abort_Association( &associationID );
        return( MERGE_FAILURE );
    }


    /*
    ** We must examine the N_CREATE response message, especially looking for
    ** the affected SOP instance UID.  This UID will be used to track the
    ** other N_SET functionality later.
    */
    status = MC_Read_Message( associationID, _timeout,
                              &rspMsgID, &rspSrvName, &rspCommand );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "sendNCREATERQ: line %d status %d. Unable to read the response message from the SCP.",
				 __LINE__, status );
        MC_Abort_Association( &associationID );
        return ( MERGE_FAILURE );
    }

    /*
    ** And then check the status of the response message.
    ** It had better be N_CREATE_SUCCESS or else we failed.
    */
    status = MC_Get_Value_To_UInt( rspMsgID, MC_ATT_STATUS, &response );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "sendNCREATERQ: line %d status %d. Unable to read the STATUS tag in the response message from the SCP.", 
				 __LINE__, status );
        MC_Abort_Association( &associationID );
        return ( MERGE_FAILURE );
    }
    if ( response != N_CREATE_SUCCESS )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "sendNCREATERQ: line %d status %d. Unexpected response status from the SCP. Response STATUS was:  %xd", 
				 __LINE__, status, response );
        MC_Abort_Association( &associationID );
        return ( MERGE_FAILURE );
    }

    /*
     * Validate and list the message
     */
    ValMessage      (rspMsgID, "N-CREATE-RSP");
    WriteToListFile (rspMsgID, "ncreatersp.msg");


    /*
    ** Note that we only look for the affected SOP instance UID and STATUS in
    ** the response message. The SCP
    ** will send back an attribute for everyone that was asked for in the
    ** request/create message.
    */
	memset(_affectedSOPinstance, 0, UI_LEN+1);
	status = MC_Get_Value_To_String( rspMsgID,
                                     MC_ATT_AFFECTED_SOP_INSTANCE_UID,
                                     UI_LEN,
                                     _affectedSOPinstance );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "sendNCREATERQ: line %d status %d. Failed to read the affected SOP instance UID in the N_CREATE_RSP message from the SCP.", 
				 __LINE__, status );
        MC_Abort_Association( &associationID );
        MC_Free_Message( &rspMsgID );
        return ( MERGE_FAILURE );
    }
    Message( MNOTE, MLoverall | toDeveloper,
			 "Received affected SOP instance UID:  %s in N_CREATE response message.", 
			 _affectedSOPinstance );

	if(strcmp(_affectedSOPinstance, p_patient->SOPInstanceUID)!=0 && strlen(p_patient->SOPInstanceUID) > 0)
        Message( MALARM, MLoverall | toDeveloper,
                 "sendNCREATERQ: line %d status %d. This SOP Instance UID is different from the one sent to it at N_CREATE:  %s. Error.",
                 __LINE__, status, p_patient->SOPInstanceUID );
	else if(strcmp(_affectedSOPinstance, p_patient->SOPInstanceUID)==0)
        Message( MNOTE, MLoverall | toDeveloper,
                 "Above returned SOP Instance UID is identical to the one sent to RIS as expected." );

    /*
    ** Although Merge SCP returns the contents of a message that has been 
    ** set, there is no guanrentee every SCP will do this, so this is not
    ** a failure condition.
    */
	memset(_mppsStatus, 0, sizeof(_mppsStatus));
    status = MC_Get_Value_To_String( rspMsgID,
                                     MC_ATT_PERFORMED_PROCEDURE_STEP_STATUS,
                                     CS_LEN,
                                     _mppsStatus );
    if ( status == MC_NORMAL_COMPLETION )
    {
        Message( MNOTE, MLoverall | toDeveloper,
				 "Acknowledge that the SCP has received the performed procedure step status: \"%s\" in N_CREATE response message.", 
				 _mppsStatus );
    }


    /*
    ** ...and then do some cleanup.
    */
    status = MC_Free_Message( &rspMsgID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "sendNCREATERQ: line %d status %d. Unable to free the N_CREATE message.  Fatal error.",
                 __LINE__, status );
        MC_Abort_Association( &associationID );
        return( MERGE_FAILURE );
    }

    /*
    ** Now that we are done, we'll close the association that
    ** we've formed with the server.
    */
    status = MC_Close_Association( &associationID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "sendNCREATERQ: line %d status %d. Unable to close the association with the MPPS SCP.", 
				 __LINE__, status );
        MC_Abort_Association( &associationID );
        return ( MERGE_FAILURE );
    }

    return( MERGE_SUCCESS );
}

/*****************************************************************************
**
** NAME
**    setNCREATERQ
**
** SYNOPSIS
**    int setNCREATERQ
**
** ARGUMENTS
**    int               p_messageID - the N_CREATE message's ID
**    WORK_PATIENT_REC *p_patient   - a pointer to a single patient record
**
** DESCRIPTION
**    This function will set all of the N_CREATE attributes for a message
**    ID that is passed in.  The message ID must be valid, and have been
**    created.
**
** RETURNS
**    MERGE_SUCCESS or MERGE_FAILURE
**
** SEE ALSO
**
*****************************************************************************/
int MppsUtils::setNCREATERQ( int p_messageID,
                      WORK_PATIENT_REC *p_patient )
{
    MC_STATUS status;
    int       schdStepAttrSeqID;
	int       schdProtocolCodeSeqID;
	int       procedureCodeSeqID;
	int       perfProtocolCodeSeqID;
	int       refStudySeqID;

    /*
    ** Here, we setup all of the needed pieces to the performed procedure
    ** step N_CREATE message.  These fields that are listed first are all of
    ** the required fields.  After these, we will fill in some type 2 fields
    ** that tie the whole Modality Worklist and Performed Procedure Step
    ** stuff together.
    **
    **
    ** First, we need some items to hold some of the attributes.
    */
    status = MC_Open_Item( &schdStepAttrSeqID, "SCHEDULED_STEP_ATTRIBUTE" );

    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to open the PROCEDURE STEP SEQUENCE item.",
				 __LINE__, status );
        return ( MERGE_FAILURE );
    }

    /*
    ** Then we place the reference to this item into the message itself.
    */
    status = MC_Set_Value_From_Int ( p_messageID,
                                     MC_ATT_SCHEDULED_STEP_ATTRIBUTES_SEQUENCE,
                                     schdStepAttrSeqID );

    if ( status != MC_NORMAL_COMPLETION )
    {
        /*
        ** Since we were unable to set the item ID into the message, we must
        ** free the item ID.  This is because the item is not
        ** yet tied to the message and freeing the message will not free this
        ** item.
        */
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to set the Scheduled Step Attributes Sequence ID: %d",
				 __LINE__, status, schdStepAttrSeqID );
        MC_Free_Item( &schdStepAttrSeqID );
		return ( MERGE_FAILURE );
    }

	/*
    ** Performed procedure step status 0x0040,0252
    */
    if ( setStringValue ( p_messageID,
                          MC_ATT_PERFORMED_PROCEDURE_STEP_STATUS,
                          "IN PROGRESS",
                          "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setStringValue for PERFORMED_PROCEDURE_STEP_STATUS: \"%s\"", 
				 __LINE__, status, "IN PROGRESS" );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

    /*
    ** Modality (0008,0060)
    */
	if ( setStringValue ( p_messageID,
                          MC_ATT_MODALITY,
                          p_patient->modality,
                          "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setStringValue for MODALITY: \"%s\"", 
				 __LINE__, status, p_patient->modality );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

	// (0040,0009)
    if ( setNullOrStringValue( schdStepAttrSeqID, MC_ATT_SCHEDULED_PROCEDURE_STEP_ID,
                               p_patient->scheduledProcedureStepID,
                               "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for SCHEDULED_PROCEDURE_STEP_ID: \"%s\"", 
				 __LINE__, status, p_patient->scheduledProcedureStepID );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

	// (0040,0007)
    if ( setNullOrStringValue( schdStepAttrSeqID,
                               MC_ATT_SCHEDULED_PROCEDURE_STEP_DESCRIPTION,
                               p_patient->scheduledProcedureStepDescription,
                               "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for SCHEDULED_PROCEDURE_STEP_DESCRIPTION: \"%s\"", 
				 __LINE__, status, p_patient->scheduledProcedureStepDescription );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

    /*
    ** Performed station AE title (0040,0241)
    */
    if ( setStringValue ( p_messageID,
                          MC_ATT_PERFORMED_STATION_AE_TITLE,
                          p_patient->performedStationAETitle,
                          "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setStringValue for PERFORMED_STATION_AE_TITLE: \"%s\"", 
				 __LINE__, status, p_patient->performedStationAETitle );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

    /*
    ** Now set the attributes that we do not have from the worklist, but
    ** are type 2, so we decide to send these due to the IHE requirements.
    */
	// (0040,0242)
    if ( setNullOrStringValue( p_messageID, MC_ATT_PERFORMED_STATION_NAME,
                               p_patient->performedStationName, 
							   "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for PERFORMED_STATION_NAME: \"%s\"", 
				 __LINE__, status, p_patient->performedStationName );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

	// (0040,0243)
    if ( setNullOrStringValue( p_messageID, MC_ATT_PERFORMED_LOCATION,
                               p_patient->performedLocation, 
							   "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for PERFORMED_LOCATION: \"%s\"", 
				 __LINE__, status, p_patient->performedLocation );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

    /*
    ** Performed procedure step start date (0040,0244).
    */
    if ( setStringValue ( p_messageID,
                          MC_ATT_PERFORMED_PROCEDURE_STEP_START_DATE,
                          p_patient->performedProcedureStepStartDate,
                          "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setStringValue for PERFORMED_PROCEDURE_STEP_START_DATE: \"%s\"", 
				 __LINE__, status, p_patient->performedProcedureStepStartDate );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

    /*
    ** Performed procedure step start time (0040,0245).
    */
    if ( setStringValue ( p_messageID,
                          MC_ATT_PERFORMED_PROCEDURE_STEP_START_TIME,
                          p_patient->performedProcedureStepStartTime,
                          "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setStringValue for PERFORMED_PROCEDURE_STEP_START_TIME: \"%s\"", 
				 __LINE__, status, p_patient->performedProcedureStepStartTime );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

    /*
    ** Performed procedure step end date and time should be blank for N_CREATE.
    */
	// (0040,0250)
    if ( setStringValue ( p_messageID,
                          MC_ATT_PERFORMED_PROCEDURE_STEP_END_DATE,
                          p_patient->performedProcedureStepEndDate,
                          "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setStringValue for PERFORMED_PROCEDURE_STEP_END_DATE: \"%s\"", 
				 __LINE__, status, p_patient->performedProcedureStepEndDate );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

    /*
    ** Performed procedure step end time (0040,0251).
    */
    if ( setStringValue ( p_messageID,
                          MC_ATT_PERFORMED_PROCEDURE_STEP_END_TIME,
                          p_patient->performedProcedureStepEndTime,
                          "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setStringValue for PERFORMED_PROCEDURE_STEP_END_TIME: \"%s\"", 
				 __LINE__, status, p_patient->performedProcedureStepEndTime );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

    /*
    ** Performed procedure step ID (0040,0253)
    */
    Message( MNOTE, MLoverall | toDeveloper,
             "performedProcedureStepID used in N_CREATE is %s\n", p_patient->performedProcedureStepID);
    if ( setStringValue ( p_messageID,
                          MC_ATT_PERFORMED_PROCEDURE_STEP_ID,
                          p_patient->performedProcedureStepID,
                          "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setStringValue for PERFORMED_PROCEDURE_STEP_ID: \"%s\"", 
				 __LINE__, status, p_patient->performedProcedureStepID );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

	// (0040,0254)
    if ( setNullOrStringValue( p_messageID,
                               MC_ATT_PERFORMED_PROCEDURE_STEP_DESCRIPTION,
                               p_patient->performedProcedureStepDescription, 
							   "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for PERFORMED_PROCEDURE_STEP_DESCRIPTION: \"%s\"", 
				 __LINE__, status, p_patient->performedProcedureStepDescription );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

	// (0040,0255)
    if ( setNullOrStringValue( p_messageID,
                               MC_ATT_PERFORMED_PROCEDURE_TYPE_DESCRIPTION,
                               p_patient->performedProcedureTypeDescription, 
							   "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for PERFORMED_PROCEDURE_TYPE_DESCRIPTION: \"%s\"", 
				 __LINE__, status, p_patient->performedProcedureTypeDescription );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

	// These attributes go to Scheduled Step Attributes Sequence
	// (0040,1001)
    if ( setNullOrStringValue( schdStepAttrSeqID, MC_ATT_REQUESTED_PROCEDURE_ID,
                               p_patient->requestedProcedureID,
                               "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for REQUESTED_PROCEDURE_ID: \"%s\"", 
				 __LINE__, status, p_patient->requestedProcedureID );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

	// (0032,1060)
    if ( setNullOrStringValue( schdStepAttrSeqID, MC_ATT_REQUESTED_PROCEDURE_DESCRIPTION,
                               p_patient->requestedProcedureDescription,
                               "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for REQUESTED_PROCEDURE_DESCRIPTION: \"%s\"", 
				 __LINE__, status, p_patient->requestedProcedureDescription );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

    /*
    ** and then, filling in some of the attributes that are part of
    ** the item:  study instance UID (0020,000D).
    */
    if ( setStringValue ( schdStepAttrSeqID,
                          MC_ATT_STUDY_INSTANCE_UID,
                          p_patient->studyInstanceUID,
                          "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setStringValue for STUDY_INSTANCE_UID: \"%s\"", 
				 __LINE__, status, p_patient->studyInstanceUID );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

	// (0008,0050)
    if ( setNullOrStringValue( schdStepAttrSeqID, MC_ATT_ACCESSION_NUMBER,
                               p_patient->accessionNumber,
                               "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for ACCESSION_NUMBER: \"%s\"", 
				 __LINE__, status, p_patient->accessionNumber );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

    /*
    ** The next few fields
    ** will be used by the MPPS SCP to tie this MPPS N_CREATE request to
    ** a particular patient, and all of that other worklist stuff.
    ** These fields aren't required by a MPPS SCP.  They are type 2.  In cases
    ** where MPPS is used with Modality Worklist, it makes sense to send the
    ** key patient demographic information that was just obtained via the
    ** worklist.  If the worklist SCP is also the MPPS SCP,
    ** the SCP requires these fields in order to "tie" the information together.
    ** If this information isn't sent, then the SCP will create an MPPS instance
    ** that is not tied to any particular patient, and will update your
    ** patient demographic information upon receipt of an N_SET.
    */

    /*
    ** First, set the attributes we retrieved from the worklist, that we 
    ** should have values for.
    */
	// (0010,0010)
    if ( setNullOrStringValue( p_messageID, MC_ATT_PATIENTS_NAME,
                               p_patient->patientsName,
                               "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for PATIENTS_NAME: \"%s\"", 
				 __LINE__, status, p_patient->patientsName );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

	// (0010,0020)
    if ( setNullOrStringValue( p_messageID, MC_ATT_PATIENT_ID,
                               p_patient->patientID,
                               "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for PATIENT_ID: \"%s\"", 
				 __LINE__, status, p_patient->patientID );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

	// (0010,0030)
    if ( setNullOrStringValue( p_messageID, MC_ATT_PATIENTS_BIRTH_DATE,
                               p_patient->patientsDOB,
                               "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for PATIENTS_BIRTH_DATE: \"%s\"", 
				 __LINE__, status, p_patient->patientsDOB );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

	// (0010,0040)
    if ( setNullOrStringValue( p_messageID, MC_ATT_PATIENTS_SEX,
                               p_patient->patientsGender,
                               "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for patientsGender: \"%s\"", 
				 __LINE__, status, p_patient->patientsGender );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

	// (0020,0010)
	if ( setNullOrStringValue( p_messageID, MC_ATT_STUDY_ID,
                               p_patient->studyID, "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for studyID: \"%s\"", 
				 __LINE__, status, p_patient->studyID );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

    /*
    ** Requested SOP instance UID. This is required by IHE, but not required in general.
    ** If an SCU doesn't send this value, then the SCP will create the UID and send it
    ** back to us. In this case, since we sent it, the SCP should send the same UID back
    ** to us.
    */
    Message( MNOTE, MLoverall | toDeveloper,
             "N_CREATE is sending requested SOP instance UID: %s", p_patient->SOPInstanceUID );
	// (0008,0018)
	if ( setStringValue ( p_messageID,
                          MC_ATT_AFFECTED_SOP_INSTANCE_UID,
                          p_patient->SOPInstanceUID,
                          "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setStringValue for SOPInstanceUID: \"%s\"", 
				 __LINE__, status, p_patient->SOPInstanceUID );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}
    
	// (0008,0005)
	if ( setStringValue ( p_messageID,
                          MC_ATT_SPECIFIC_CHARACTER_SET,
                          p_patient->specificCharacterSet,
                          "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setStringValue for specificCharacterSet: \"%s\"", 
				 __LINE__, status, p_patient->specificCharacterSet );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}
    
	// We do not insert any values from NSETImageUIDs because they are for N_SET command

	if ( strlen(p_patient->protocolCodeValue)>0 &&
		 strlen(p_patient->protocolCodingScheme)>0 )
	{
	    /*
	    ** Now we open the Scheduled Protocol Code Sequence item (0040,0008), 
		** that are part of the Scheduled Step Attribute Sequence (0040,0270).
	    */
	    status = MC_Open_Item( &schdProtocolCodeSeqID,
			                   "SCHEDULED_PROTOCOL_CODE" );
		if ( status != MC_NORMAL_COMPLETION )
	    {
		    Message( MWARNING, MLoverall | toDeveloper,
			         "setNCREATERQ: line %d status %d. Failed to open the Scheduled Protocol Code Sequence item.",
				     __LINE__, status );
	        MC_Free_Item( &schdStepAttrSeqID );
			return ( MERGE_FAILURE );
		}
	    /*
		** And place reference to the sequence item (0040,0008) in its respective place.
	    */
		status = MC_Set_Value_From_Int( schdStepAttrSeqID, 
			                            MC_ATT_SCHEDULED_ACTION_ITEM_CODE_SEQUENCE,
				                        schdProtocolCodeSeqID );
	    if ( status != MC_NORMAL_COMPLETION )
		{
			/*
	        ** Since we can't place the item into the message, we need to free
	        ** the item before we return with an error.
		    */
			Message( MWARNING, MLoverall | toDeveloper,
				     "setNCREATERQ: line %d status %d. Failed to set the Scheduled Protocol Code Sequence item.",
					 __LINE__, status );
	        MC_Free_Item( &schdProtocolCodeSeqID );
		    MC_Free_Item( &schdStepAttrSeqID );
			return ( MERGE_FAILURE );
	    }

		// (0008,0100)
		if ( setStringValue( schdProtocolCodeSeqID, MC_ATT_CODE_VALUE,
		                     p_patient->protocolCodeValue, "setNCREATERQ" ) != MERGE_SUCCESS )
		{		
			Message( MWARNING, MLoverall | toDeveloper,
				     "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for protocolCodeValue: \"%s\"", 
					 __LINE__, status, p_patient->protocolCodeValue );
		    MC_Free_Item( &schdProtocolCodeSeqID );
			MC_Free_Item( &schdStepAttrSeqID );
		    return ( MERGE_FAILURE );
		}

		// (0008,0103)
		if ( setStringValue( schdProtocolCodeSeqID, MC_ATT_CODING_SCHEME_DESIGNATOR,
			                 p_patient->protocolCodingScheme, "setNCREATERQ" ) != MERGE_SUCCESS )
		{		
			Message( MWARNING, MLoverall | toDeveloper,
				     "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for protocolCodingScheme: \"%s\"", 
					 __LINE__, status, p_patient->protocolCodingScheme );
		    MC_Free_Item( &schdProtocolCodeSeqID );
			MC_Free_Item( &schdStepAttrSeqID );
		    return ( MERGE_FAILURE );
		}

		if ( strlen(p_patient->protocolCodeMeaning)>0 )
		{
			// (0008,0104)
			if ( setStringValue( schdProtocolCodeSeqID, MC_ATT_CODE_MEANING,
					             p_patient->protocolCodeMeaning, "setNCREATERQ" ) != MERGE_SUCCESS )
			{
				Message( MWARNING, MLoverall | toDeveloper,
						 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for protocolCodeMeaning: \"%s\"", 
						 __LINE__, status, p_patient->protocolCodeMeaning );
				MC_Free_Item( &schdProtocolCodeSeqID );
				MC_Free_Item( &schdStepAttrSeqID );
				return ( MERGE_FAILURE );
			}
		}

	    MC_Free_Item( &schdProtocolCodeSeqID );
	}

	if ( strlen(p_patient->procedureCodeValue)>0 &&
		 strlen(p_patient->procedureCodingScheme)>0 )
	{
		/*
		** Now we open the Procedure Code Sequence item (0008,1032), 
		** that is part of the main message.
		*/
		status = MC_Open_Item( &procedureCodeSeqID,
		                       "PROCEDURE_CODE" );
		if ( status != MC_NORMAL_COMPLETION )
		{
			Message( MWARNING, MLoverall | toDeveloper,
				     "setNCREATERQ: line %d status %d. Failed to open the Procedure Code Sequence item.",
					 __LINE__, status );
			MC_Free_Item( &schdStepAttrSeqID );
			return ( MERGE_FAILURE );
		}
	    /*
		** And place reference to the sequence item (0008,1032) in its respective place.
		*/
		status = MC_Set_Value_From_Int( p_messageID, 
			                            MC_ATT_PROCEDURE_CODE_SEQUENCE,
				                        procedureCodeSeqID );
		if ( status != MC_NORMAL_COMPLETION )
		{
			/*
			** Since we can't place the item into the message, we need to free
			** the item before we return with an error.
			*/
			Message( MWARNING, MLoverall | toDeveloper,
				     "setNCREATERQ: line %d status %d. Failed to set the Procedure Code Sequence item.",
					 __LINE__, status );
			MC_Free_Item( &schdStepAttrSeqID );
			MC_Free_Item( &procedureCodeSeqID );
			return ( MERGE_FAILURE );
		}

		if ( setStringValue( procedureCodeSeqID, MC_ATT_CODE_VALUE,
		                     p_patient->procedureCodeValue, "setNCREATERQ" ) != MERGE_SUCCESS )
		{		
			Message( MWARNING, MLoverall | toDeveloper,
				     "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for procedureCodeValue: \"%s\"", 
					 __LINE__, status, p_patient->procedureCodeValue );
			MC_Free_Item( &schdStepAttrSeqID );
		    MC_Free_Item( &procedureCodeSeqID );
			return ( MERGE_FAILURE );
		}

		if ( setStringValue( procedureCodeSeqID, MC_ATT_CODING_SCHEME_DESIGNATOR,
			                 p_patient->procedureCodingScheme, "setNCREATERQ" ) != MERGE_SUCCESS )
		{		
			Message( MWARNING, MLoverall | toDeveloper,
				     "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for procedureCodingScheme: \"%s\"", 
					 __LINE__, status, p_patient->procedureCodingScheme );
			MC_Free_Item( &schdStepAttrSeqID );
		    MC_Free_Item( &procedureCodeSeqID );
	        return ( MERGE_FAILURE );
		}

		if ( strlen(p_patient->procedureCodeMeaning)>0 )
		{
			if ( setStringValue( procedureCodeSeqID, MC_ATT_CODE_MEANING,
					             p_patient->procedureCodeMeaning, "setNCREATERQ" ) != MERGE_SUCCESS )
			{		
		        Message( MWARNING, MLoverall | toDeveloper,
				         "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for procedureCodeMeaning: \"%s\"", 
						 __LINE__, status, p_patient->procedureCodeMeaning );
				MC_Free_Item( &schdStepAttrSeqID );
			    MC_Free_Item( &procedureCodeSeqID );
			    return ( MERGE_FAILURE );
			}
		}

	    MC_Free_Item( &procedureCodeSeqID );
	}

	if ( strlen(p_patient->performedProtocolCodeValue)>0 &&
		 strlen(p_patient->performedProtocolCodingScheme)>0 )
	{
		/*
		** Now we open the Performed Protocol Code Sequence item (0040,0260), 
		** that is part of the main message.
		*/
		status = MC_Open_Item( &perfProtocolCodeSeqID,
			                   "PERFORMED_PROTOCOL_CODE" );
		if ( status != MC_NORMAL_COMPLETION )
		{
		    Message( MWARNING, MLoverall | toDeveloper,
			         "setNCREATERQ: line %d status %d. Failed to open the Performed Protocol Code Sequence item.",
				     __LINE__, status );
			MC_Free_Item( &schdStepAttrSeqID );
			return ( MERGE_FAILURE );
		}
		/*
		** And place reference to the sequence item (0040,0260) in its respective place.
		*/
		status = MC_Set_Value_From_Int( p_messageID, 
			                            MC_ATT_PERFORMED_ACTION_ITEM_SEQUENCE,
				                        perfProtocolCodeSeqID );
		if ( status != MC_NORMAL_COMPLETION )
		{
			/*
			** Since we can't place the item into the message, we need to free
			** the item before we return with an error.
			*/
			Message( MWARNING, MLoverall | toDeveloper,
			         "setNCREATERQ: line %d status %d. Failed to set the Performed Protocol Code Sequence item.",
			         __LINE__, status );
			MC_Free_Item( &schdStepAttrSeqID );
			MC_Free_Item( &perfProtocolCodeSeqID );
			return ( MERGE_FAILURE );
		}

		if ( setStringValue( perfProtocolCodeSeqID, MC_ATT_CODE_VALUE,
		                     p_patient->performedProtocolCodeValue, "setNCREATERQ" ) != MERGE_SUCCESS )
		{		
			Message( MWARNING, MLoverall | toDeveloper,
				     "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for performedProtocolCodeValue: \"%s\"", 
					 __LINE__, status, p_patient->performedProtocolCodeValue );
			MC_Free_Item( &schdStepAttrSeqID );
		    MC_Free_Item( &perfProtocolCodeSeqID );
		    return ( MERGE_FAILURE );
		}

		if ( setStringValue( perfProtocolCodeSeqID, MC_ATT_CODING_SCHEME_DESIGNATOR,
		                     p_patient->performedProtocolCodingScheme, "setNCREATERQ" ) != MERGE_SUCCESS )
		{		
			Message( MWARNING, MLoverall | toDeveloper,
				     "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for performedProtocolCodingScheme: \"%s\"", 
					 __LINE__, status, p_patient->performedProtocolCodingScheme );
			MC_Free_Item( &schdStepAttrSeqID );
		    MC_Free_Item( &perfProtocolCodeSeqID );
		    return ( MERGE_FAILURE );
		}

		if ( strlen(p_patient->performedProtocolCodeMeaning)>0 )
		{
			if ( setStringValue( perfProtocolCodeSeqID, MC_ATT_CODE_MEANING,
					             p_patient->performedProtocolCodeMeaning, "setNCREATERQ" ) != MERGE_SUCCESS )
			{		
				Message( MWARNING, MLoverall | toDeveloper,
		                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for performedProtocolCodeMeaning: \"%s\"", 
						 __LINE__, status, p_patient->performedProtocolCodeMeaning );
				MC_Free_Item( &schdStepAttrSeqID );
		        MC_Free_Item( &perfProtocolCodeSeqID );
		        return ( MERGE_FAILURE );
			}
		}

	    MC_Free_Item( &perfProtocolCodeSeqID );
	}

	if ( strlen(p_patient->referencedSOPInstanceUID)>0 )
	{
		/*
		** Now we open the Referenced Study Sequence item (0008,1110), 
		** that is part of the main message.
		*/
		status = MC_Open_Item( &refStudySeqID,
			                   "REFERENCED_STUDY" );
		if ( status != MC_NORMAL_COMPLETION )
		{
			Message( MWARNING, MLoverall | toDeveloper,
				     "setNCREATERQ: line %d status %d. Failed to open the Referenced Study Sequence item.",
					 __LINE__, status );
			MC_Free_Item( &schdStepAttrSeqID );
			return ( MERGE_FAILURE );
		}
		/*
		** And place reference to the sequence item (0008,1110) in its respective place.
		*/
		status = MC_Set_Value_From_Int( schdStepAttrSeqID,
			                            MC_ATT_REFERENCED_STUDY_SEQUENCE,
				                        refStudySeqID );
		if ( status != MC_NORMAL_COMPLETION )
		{
			/*
		    ** Since we can't place the item into the message, we need to free
		    ** the item before we return with an error.
			*/
		    Message( MWARNING, MLoverall | toDeveloper,
		             "setNCREATERQ: line %d status %d. Failed to set the Referenced Study Sequence item.",
			         __LINE__, status );
		    MC_Free_Item( &refStudySeqID );
			MC_Free_Item( &schdStepAttrSeqID );
		    return ( MERGE_FAILURE );
		}

		// (0008,1110)>0x0008,1150
		if ( setNullOrStringValue( refStudySeqID, MC_ATT_REFERENCED_SOP_CLASS_UID,
			                       p_patient->referencedSOPClassUID, "setNCREATERQ" ) != MERGE_SUCCESS )
		{		
		    Message( MWARNING, MLoverall | toDeveloper,
			         "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for referencedSOPClassUID: \"%s\"", 
					 __LINE__, status, p_patient->referencedSOPClassUID );
			MC_Free_Item( &refStudySeqID );
			MC_Free_Item( &schdStepAttrSeqID );
			return ( MERGE_FAILURE );
		}

		// (0008,1110)>0x0008,1155
		if ( setNullOrStringValue( refStudySeqID, MC_ATT_REFERENCED_SOP_INSTANCE_UID,
			                       p_patient->referencedSOPInstanceUID, "setNCREATERQ" ) != MERGE_SUCCESS )
		{		
		    Message( MWARNING, MLoverall | toDeveloper,
			         "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for referencedSOPInstanceUID: \"%s\"", 
					 __LINE__, status, p_patient->referencedSOPInstanceUID );
			MC_Free_Item( &refStudySeqID );
			MC_Free_Item( &schdStepAttrSeqID );
		    return ( MERGE_FAILURE );
		}

		MC_Free_Item( &refStudySeqID );
	}
	else
	{
		if ( setNullOrStringValue( schdStepAttrSeqID, MC_ATT_REFERENCED_STUDY_SEQUENCE,
			                       "", "setNCREATERQ" ) != MERGE_SUCCESS )
		{		
		    Message( MWARNING, MLoverall | toDeveloper,
			         "setNCREATERQ: line %d status %d. Failed to Null Value for REFERENCED_STUDY_SEQUENCE", 
					 __LINE__, status );
			MC_Free_Item( &schdStepAttrSeqID );
		    return ( MERGE_FAILURE );
		}
	}

	/*
    ** Now, set several sequence attributes to NULL that are required,
    */

	// Scheduled Protocol Code Sequence 0x0040,0008
	if ( setNullOrStringValue( schdStepAttrSeqID,
                               MC_ATT_SCHEDULED_ACTION_ITEM_CODE_SEQUENCE,
                               "", "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for SCHEDULED_ACTION_ITEM_CODE_SEQUENCE: \"%s\"", 
				 __LINE__, status, "" );
		MC_Free_Item( &schdStepAttrSeqID );
        return ( MERGE_FAILURE );
	}

	MC_Free_Item( &schdStepAttrSeqID );
    
	// Referenced Patient Sequence 0x0008,1120
    if ( setNullOrStringValue( p_messageID, MC_ATT_REFERENCED_PATIENT_SEQUENCE,
                               "", "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for REFERENCED_PATIENT_SEQUENCE: \"%s\"", 
				 __LINE__, status, "" );
        return ( MERGE_FAILURE );
	}
    
	if ( strlen(p_patient->procedureCodeValue)==0 ||
		 strlen(p_patient->procedureCodingScheme)==0 )
	{
		if ( setNullOrStringValue( p_messageID, MC_ATT_PROCEDURE_CODE_SEQUENCE,
			                       "", "setNCREATERQ" ) != MERGE_SUCCESS )
		{		
			Message( MWARNING, MLoverall | toDeveloper,
				     "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for PROCEDURE_CODE_SEQUENCE:  \"%s\"", 
					 __LINE__, status, "" );
			return ( MERGE_FAILURE );
		}
	}

	// Performed Protocol Code Sequence 0x0040,0260
	if ( setNullOrStringValue( p_messageID,
                               MC_ATT_PERFORMED_ACTION_ITEM_SEQUENCE,
                               "", "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for PERFORMED_ACTION_ITEM_SEQUENCE: \"%s\"", 
				 __LINE__, status, "" );
        return ( MERGE_FAILURE );
	}
    
    if ( setNullOrStringValue( p_messageID, MC_ATT_PERFORMED_SERIES_SEQUENCE,
                               "", "setNCREATERQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNCREATERQ: line %d status %d. Failed to setNullOrStringValue for PERFORMED_SERIES_SEQUENCE:  \"%s\"", 
				 __LINE__, status, "" );
        return ( MERGE_FAILURE );
	}

	/*
    ** performingPhysiciansName, protocolName and other attributes under PERFORMED_SERIES_SEQUENCE
	** are not required if PERFORMED_SERIES_SEQUENCE is not present...
    */

    return( MERGE_SUCCESS );
}

/*****************************************************************************
**
** NAME
**    sendNSETRQStatus
**
** SYNOPSIS
**    int sendNSETRQStatus
**
** ARGUMENTS
**    int               p_applicationID        - the application ID
**    WORK_PATIENT_REC *p_patient              - a patient record
**    MPPS_STATUS       p_status               - Take 1/2 values: MPPS_COMPLETED or MPPS_DISCONTINUED
**
** DESCRIPTION
**    This function updates the status of a modality performed procedure
**    step at this modality.
**
** RETURNS
**    MERGE_SUCCESS or MERGE_FAILURE
**
** SEE ALSO
**    
*****************************************************************************/
int MppsUtils::sendNSETRQStatus(	int p_applicationID,
									WORK_PATIENT_REC *p_patient,
									MPPS_STATUS p_status)
{
    MC_STATUS  status;
    int        associationID;
    int        messageID;
    int        rspMsgID;
    char       *rspSrvName;
    MC_COMMAND rspCommand;

    /*
    ** In order to send the PERFORMED PROCEDURE STEP N_SET message to the
    ** SCP, we need to open a message.
    */
    status = MC_Open_Message( &messageID, "PERFORMED_PROCEDURE_STEP",
                              N_SET_RQ );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "sendNSETRQStatus: line %d status %d. Unable to open a message for PERFORMED_PROCEDURE_STEP N_SET operation",
				 __LINE__, status );
		return ( MERGE_FAILURE );
    }

    /*
    ** Now, we setup all of the needed pieces to the performed procedure
    ** step N_SET message.
    */

	if ( setNSETRQ (messageID, p_patient) != MERGE_SUCCESS )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "sendNSETRQStatus: line %d status %d. Unable to setNSETRQ",
                 __LINE__, status );
	    MC_Free_Message(&messageID);
		return ( MERGE_FAILURE );
	}

    /* 
    ** And finally, we set the status to completed or discontinued.
    */
    if ( p_status == MPPS_COMPLETED )
	{
		if ( setStringValue ( messageID,
		                      MC_ATT_PERFORMED_PROCEDURE_STEP_STATUS,
			                  "COMPLETED",
				              "sendNSETRQStatus" ) != MERGE_SUCCESS )
	    {
            Message( MWARNING, MLoverall | toDeveloper,
                     "sendNSETRQStatus: line %d status %d. Unable to setStringValue \"COMPLETED\" for PERFORMED_PROCEDURE_STEP_STATUS.",
                     __LINE__, status );
		    MC_Free_Message(&messageID);
			return ( MERGE_FAILURE );
		}
	}
	else if ( p_status == MPPS_DISCONTINUED )
	{
		if ( setStringValue ( messageID,
		                      MC_ATT_PERFORMED_PROCEDURE_STEP_STATUS,
			                  "DISCONTINUED",
				              "sendNSETRQStatus" ) != MERGE_SUCCESS )
	    {
            Message( MWARNING, MLoverall | toDeveloper,
                     "sendNSETRQStatus: line %d status %d. Unable to setStringValue \"DISCONTINUED\" for PERFORMED_PROCEDURE_STEP_STATUS.",
                     __LINE__, status );
		    MC_Free_Message(&messageID);
			return ( MERGE_FAILURE );
		}
	}

    /*
    ** Then, we need to open an association with a modality performed
    ** procedure step SCP.
    */
	try {
		associationID = openAssociation( p_applicationID );
	}
	catch (...)
	{
        MC_Free_Message( &messageID );
        Message( MWARNING, MLoverall | toDeveloper,
                 "sendNSETRQStatus: line %d status %d. openAssociation failed.", __LINE__, status );
        return( MERGE_FAILURE );
    }
    
    /*
     * Validate and list the message
     */
    ValMessage      (messageID, "N-SET-RQ Status");
    WriteToListFile (messageID, "nsetrqcmp.msg");

    /*
    ** And finally, we send the N_SET message to the server.
    */
    status = MC_Send_Request_Message( associationID, messageID );
    if ( status != MC_NORMAL_COMPLETION )
    {
	    if ( p_status == MPPS_COMPLETED )
		{
			Message( MWARNING, MLoverall | toDeveloper,
				     "sendNSETRQStatus: line %d status %d. Problem to send the N_SET MPPS_COMPLETED message to the SCP.", __LINE__, status );
		}
		else if ( p_status == MPPS_DISCONTINUED )
		{
			Message( MWARNING, MLoverall | toDeveloper,
				     "sendNSETRQStatus: line %d status %d. Problem to send the N_SET MPPS_DISCONTINUED message to the SCP.", __LINE__, status );
		}
        MC_Free_Message ( &messageID );
        MC_Abort_Association ( &associationID );
        return ( MERGE_FAILURE );
    }

    /*
    ** Free up the request message.
    */
    status = MC_Free_Message( &messageID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "sendNSETRQStatus: line %d status %d. Unable to free the N_SET message.  Fatal error.",
                 __LINE__, status );
        MC_Abort_Association ( &associationID );
        return ( MERGE_FAILURE );
    }

    /*
    ** We can examine the N_SET response message, verifying that the status
    ** comes back as expected.
    */
    status = MC_Read_Message( associationID, _timeout,
                              &rspMsgID, &rspSrvName, &rspCommand );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "sendNSETRQStatus: line %d status %d. Unable to read the response message from the SCP.", 
				 __LINE__, status );
        MC_Abort_Association( &associationID );
        return ( MERGE_FAILURE );
    }
    
	memset(_mppsStatus, 0, sizeof(_mppsStatus));
    status = MC_Get_Value_To_String( rspMsgID,
                                     MC_ATT_PERFORMED_PROCEDURE_STEP_STATUS,
                                     CS_LEN,
                                     _mppsStatus );
    if ( status == MC_NORMAL_COMPLETION )
    {
        Message( MNOTE, MLoverall | toDeveloper,
				"Acknowledge that the SCP has received the performed procedure step status:  %s in N_SET response message.", _mppsStatus );
    }


    /*
    ** Now that we are done, we'll close the association that
    ** we've formed with the server.
    */
    status = MC_Close_Association( &associationID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "sendNSETRQStatus: line %d status %d. Unable to close the association with the MPPS SCP.", 
				 __LINE__, status );
        MC_Free_Message( &rspMsgID );
        return( MERGE_FAILURE );
    }

    /*
    ** ...and then do some cleanup.
    */
    status = MC_Free_Message( &rspMsgID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "sendNSETRQStatus: line %d status %d. Unable to free the N_SET	response message. Fatal error.",
                 __LINE__, status );
        return( MERGE_FAILURE );
    }

    return( MERGE_SUCCESS );
}

/*****************************************************************************
**
** NAME
**    setNSETRQ
**
** SYNOPSIS
**    int setNSETRQ
**
** ARGUMENTS
**    int   p_messageID           - the N_SET message ID
**    WORK_PATIENT_REC *p_patient - the patient info to be set
**
** DESCRIPTION
**    This function will set all of the necessary attributes for an N_SET
**    response message.
**
** RETURNS
**    MERGE_SUCCESS or MERGE_FAILURE
**
** SEE ALSO
**
*****************************************************************************/
int MppsUtils::setNSETRQ(
             int   p_messageID,
             WORK_PATIENT_REC *p_patient
             )
{
    MC_STATUS status;
    int       perfSeriesSeqID;
    int       refImageID;
    char      date[ 9 ]="";
    char      time[ 9 ]="";
	char      NSETImageUIDs[ UI_LEN*64+1 ];			// UIDs for (0040,0340)>(0008,1140)>(0008,1155)
    char      *begin, *end;
	int       procedureCodeSeqID;
	int       perfProtocolCodeSeqID;
    list<AcqImageRef>::iterator i;

    /* 
    ** A group zero element:  requested SOP instance UID.  This UID will have
    ** been returned from the SCP during the N_CREATE call by this SCU to the
    ** SCP. It is basically a key, created by the SCP that is used by the
    ** SCP to know which data set that the SCU is referencing during the N_SET.
    */
	// (0008,0018)
    if ( setStringValue ( p_messageID,
                          MC_ATT_REQUESTED_SOP_INSTANCE_UID,
                          p_patient->SOPInstanceUID,
                          "setNSETRQ" ) != MERGE_SUCCESS )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNSETRQ: line %d. Unable to setStringValue \"%s\" for SOPInstanceUID.",
                 __LINE__, p_patient->SOPInstanceUID );
        return ( MERGE_FAILURE );
    }

	// (0008,0005)
    if ( setStringValue ( p_messageID,
                          MC_ATT_SPECIFIC_CHARACTER_SET,
                          p_patient->specificCharacterSet,
                          "setNSETRQ" ) != MERGE_SUCCESS )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNSETRQ: line %d. Unable to setStringValue \"%s\" for specificCharacterSet.",
                 __LINE__, p_patient->specificCharacterSet );
        return ( MERGE_FAILURE );
    }

	/*
    ** Performed procedure step end date
    ** Both the end date and time are obtained at this point because this signifies
    ** the actual end of the procedure step. But use the values if provided.
    */
    GetCurrentDate( date, time );
	if ( strlen(p_patient->performedProcedureStepEndDate) > 0)
		strncpy(date, p_patient->performedProcedureStepEndDate, sizeof(date));

	// (0040,0250)
	if ( setStringValue ( p_messageID,
                          MC_ATT_PERFORMED_PROCEDURE_STEP_END_DATE,
                          date,
                          "setNSETRQ" ) != MERGE_SUCCESS )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNSETRQ: line %d. Unable to setStringValue \"%s\" for PERFORMED_PROCEDURE_STEP_END_DATE.",
                 __LINE__, date );
        return ( MERGE_FAILURE );
    }

    /*
    ** Performed procedure step end time
    */
	if ( strlen(p_patient->performedProcedureStepEndTime) > 0)
		strncpy(time, p_patient->performedProcedureStepEndTime, sizeof(time));

	// (0040,0251)
    if ( setStringValue ( p_messageID,
                          MC_ATT_PERFORMED_PROCEDURE_STEP_END_TIME,
                          time,
                          "setNSETRQ" ) != MERGE_SUCCESS )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNSETRQ: line %d. Unable to setStringValue \"%s\" for PERFORMED_PROCEDURE_STEP_END_TIME.",
                 __LINE__, time );
        return ( MERGE_FAILURE );
    }

	// (0040,0254)
    if ( setNullOrStringValue( p_messageID,
                               MC_ATT_PERFORMED_PROCEDURE_STEP_DESCRIPTION,
                               p_patient->performedProcedureStepDescription, 
							   "setNSETRQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNSETRQ: line %d. Failed to setNullOrStringValue for PERFORMED_PROCEDURE_STEP_DESCRIPTION: \"%s\"", 
				 __LINE__, p_patient->performedProcedureStepDescription );
        return ( MERGE_FAILURE );
	}

	// (0040,0255)
    if ( setNullOrStringValue( p_messageID,
                               MC_ATT_PERFORMED_PROCEDURE_TYPE_DESCRIPTION,
                               p_patient->performedProcedureTypeDescription, 
							   "setNSETRQ" ) != MERGE_SUCCESS )
	{		
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNSETRQ: line %d. Failed to setNullOrStringValue for PERFORMED_PROCEDURE_TYPE_DESCRIPTION: \"%s\"", 
				 __LINE__, p_patient->performedProcedureTypeDescription );
        return ( MERGE_FAILURE );
	}

	/*
    ** Now we need some items, that are part of the main message.
    */
    status = MC_Open_Item( &perfSeriesSeqID,
                           "PERFORMED_SERIES" );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNSETRQ: line %d status %d. Failed to open the performed series sequence item.",
                 __LINE__, status );
        return ( MERGE_FAILURE );
    }
    /*
    ** And place reference to the sequence item in its respective place.
    */
    status = MC_Set_Value_From_Int( p_messageID, 
                                    MC_ATT_PERFORMED_SERIES_SEQUENCE,
                                    perfSeriesSeqID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        /*
        ** Since we can't place the item into the message, we need to free
        ** the item before we return with an error.
        */
        Message( MWARNING, MLoverall | toDeveloper,
                 "setNSETRQ: line %d status %d. Failed to set the performed series sequence item.",
                 __LINE__, status );
        MC_Free_Item( &perfSeriesSeqID );
        return ( MERGE_FAILURE );
    }

    for(i=p_patient->acqImageRefs.begin(); i != p_patient->acqImageRefs.end();)
    {
        strncpy(NSETImageUIDs, i->NSETImageUIDs, sizeof(NSETImageUIDs));
        NSETImageUIDs[sizeof(NSETImageUIDs)-1] = '\0';
        begin = NSETImageUIDs;

        if ( strlen(begin) > 0 )
        {
            status = MC_Open_Item( &refImageID,
                                   "REF_IMAGE" );
            if ( status != MC_NORMAL_COMPLETION )
            {
                Message( MWARNING, MLoverall | toDeveloper,
                         "setNSETRQ: line %d status %d. Failed to open the referenced image sequence item.",
                         __LINE__, status );
                MC_Free_Item( &perfSeriesSeqID );
                return ( MERGE_FAILURE );
            }
            /*
            ** And place reference to the sequence item in its respective place.
            */
            status = MC_Set_Value_From_Int( perfSeriesSeqID,
                                            MC_ATT_REFERENCED_IMAGE_SEQUENCE,
                                            refImageID );
            if ( status != MC_NORMAL_COMPLETION )
            {
                /*
                ** Since we can't place the item into the message, we need to free
                ** the item before we return with an error.
                */
                Message( MWARNING, MLoverall | toDeveloper,
                         "setNSETRQ: line %d status %d. Failed to set the referenced image sequence item.",
                         __LINE__, status );
                MC_Free_Item( &perfSeriesSeqID );
                MC_Free_Item( &refImageID );
                return ( MERGE_FAILURE );
            }
		}

        while ( strlen(begin) > 0 )
        {
            if( (end = strchr(begin, ' ')) != NULL )
                *end = '\0';

            /*
            ** Now, we're ready for the referenced SOP class UID.  In real life, this
            ** is the SOP class UID that the modality is attempting to USE.
            */
	    	// (0008,1150)
            if ( setStringValue ( refImageID,
                                  MC_ATT_REFERENCED_SOP_CLASS_UID,
                                  ReferencedSOPClassUID,
                                  "setNSETRQ" ) != MERGE_SUCCESS )
            {
                Message( MWARNING, MLoverall | toDeveloper,
                         "setNSETRQ: line %d status %d. Unable to setStringValue \"%s\" for MC_ATT_REFERENCED_SOP_CLASS_UID.",
                         __LINE__, status, ReferencedSOPClassUID );
                MC_Free_Item( &refImageID );
                MC_Free_Item( &perfSeriesSeqID );
                return ( MERGE_FAILURE );
            }

    		// (0008,1155)
            if ( setStringValue ( refImageID,
                                  MC_ATT_REFERENCED_SOP_INSTANCE_UID,
                                  begin,
                                  "setNSETRQ" ) != MERGE_SUCCESS )
            {
                Message( MWARNING, MLoverall | toDeveloper,
                         "setNSETRQ: line %d status %d. Unable to setStringValue \"%s\" for MC_ATT_REFERENCED_SOP_INSTANCE_UID.",
                         __LINE__, status, begin );
                MC_Free_Item( &refImageID );
                MC_Free_Item( &perfSeriesSeqID );
                return ( MERGE_FAILURE );
            }

    	    if (end != NULL)
	    	{
                begin = end+1;

                status = MC_Free_Item( &refImageID );
                if ( status != MC_NORMAL_COMPLETION )
                {
                    Message( MWARNING, MLoverall | toDeveloper,
                             "setNSETRQ: line %d status %d. Failed to free the referenced image sequence item.",
                             __LINE__, status );
                    MC_Free_Item( &perfSeriesSeqID );
                    return ( MERGE_FAILURE );
                }

    			status = MC_Open_Item( &refImageID,
                                       "REF_IMAGE" );
                if ( status != MC_NORMAL_COMPLETION )
                {
                    Message( MWARNING, MLoverall | toDeveloper,
                             "setNSETRQ: line %d status %d. Failed to open the referenced image sequence item.",
                             __LINE__, status );
                    MC_Free_Item( &perfSeriesSeqID );
                    return ( MERGE_FAILURE );
                }

                /*
                 ** And place reference to the sequence item in its respective place.
                 */
                status = MC_Set_Next_Value_From_Int( perfSeriesSeqID,
                                                     MC_ATT_REFERENCED_IMAGE_SEQUENCE,
                                                     refImageID );
                if ( status != MC_NORMAL_COMPLETION )
                {
                    /*
                     ** Since we can't place the item into the message, we need to free
                     ** the item before we return with an error.
                     */
                    Message( MWARNING, MLoverall | toDeveloper,
                             "setNSETRQ: line %d status %d. Failed to set the referenced image sequence item.",
                             __LINE__, status );
                    MC_Free_Item( &refImageID );
                    MC_Free_Item( &perfSeriesSeqID );
                    return ( MERGE_FAILURE );
                }
		    }
    		else
	    	    *begin = '\0';
    	}

    	MC_Free_Item( &refImageID );
	
	    // Series Instance UID (0020,000E) goes to Performed Series Sequence
    	if ( setNullOrStringValue( perfSeriesSeqID, MC_ATT_SERIES_INSTANCE_UID,
                                   i->seriesInstanceUID, "setNSETRQ" ) != MERGE_SUCCESS )
    	{
            Message( MWARNING, MLoverall | toDeveloper,
                     "setNSETRQ: line %d status %d. Failed to setNullOrStringValue for seriesInstanceUID: \"%s\"", 
			    	 __LINE__, status, i->seriesInstanceUID );
            MC_Free_Item( &perfSeriesSeqID );
            return ( MERGE_FAILURE );
    	}

	    /*
        ** Since we are adding an image record, we need to add all of the tags
        ** that are needed for the performed series sequence item.
        */
	    // (0008,1050)
        if ( setStringValue ( perfSeriesSeqID,
                              MC_ATT_PERFORMING_PHYSICIANS_NAME,
                              p_patient->performingPhysiciansName,
                              "setNSETRQ" ) != MERGE_SUCCESS )
        {
            Message( MWARNING, MLoverall | toDeveloper,
                     "setNSETRQ: line %d status %d. Unable to setStringValue \"%s\" for MC_ATT_PERFORMING_PHYSICIANS_NAME.",
                     __LINE__, status, p_patient->performingPhysiciansName );
            MC_Free_Item( &perfSeriesSeqID );
            return ( MERGE_FAILURE );
        }

	    // (0018,1030)
        if ( setStringValue ( perfSeriesSeqID,
                              MC_ATT_PROTOCOL_NAME,
                              p_patient->protocolName,
                              "setNSETRQ" ) != MERGE_SUCCESS )
        {
            Message( MWARNING, MLoverall | toDeveloper,
                     "setNSETRQ: line %d status %d. Unable to setStringValue \"%s\" for MC_ATT_PROTOCOL_NAME.",
                     __LINE__, status, p_patient->protocolName );
            MC_Free_Item( &perfSeriesSeqID );
            return ( MERGE_FAILURE );
        }

    	// (0008,1070)
        if ( setStringValue ( perfSeriesSeqID,
                              MC_ATT_OPERATORS_NAME,
                              p_patient->operatorsName,
                              "setNSETRQ" ) != MERGE_SUCCESS )
        {
            Message( MWARNING, MLoverall | toDeveloper,
                     "setNSETRQ: line %d status %d. Unable to setStringValue \"%s\" for MC_ATT_OPERATORS_NAME.",
                     __LINE__, status, p_patient->operatorsName );
            MC_Free_Item( &perfSeriesSeqID );
            return ( MERGE_FAILURE );
        }

	    // (0008,103E)
        if ( setStringValue ( perfSeriesSeqID,
                              MC_ATT_SERIES_DESCRIPTION,
                              p_patient->seriesDescription,
                              "setNSETRQ" ) != MERGE_SUCCESS )
        {
            Message( MWARNING, MLoverall | toDeveloper,
                     "setNSETRQ: line %d status %d. Unable to setStringValue \"%s\" for MC_ATT_SERIES_DESCRIPTION.",
                     __LINE__, status, p_patient->seriesDescription );
            MC_Free_Item( &perfSeriesSeqID );
            return ( MERGE_FAILURE );
        }

    	// (0008,0054)
        if ( setStringValue ( perfSeriesSeqID,
                              MC_ATT_RETRIEVE_AE_TITLE,
                              p_patient->retrieveAETitle,
                              "setNSETRQ" ) != MERGE_SUCCESS )
        {
            Message( MWARNING, MLoverall | toDeveloper,
                     "setNSETRQ: line %d status %d. Unable to setStringValue \"%s\" for MC_ATT_RETRIEVE_AE_TITLE.",
                     __LINE__, status, p_patient->retrieveAETitle );
            MC_Free_Item( &perfSeriesSeqID );
            return ( MERGE_FAILURE );
        }

    	// (0040,0220)
        if ( MC_Set_Value_To_NULL( perfSeriesSeqID,
                                 MC_ATT_REFERENCED_STANDALONE_SOP_INSTANCE_SEQUENCE ) != MC_NORMAL_COMPLETION )
        {
            Message( MWARNING, MLoverall | toDeveloper,
                     "setNSETRQ: line %d status %d. Failed to set the referenced standalone SOP instance sequence to NULL.",
                     __LINE__, status );
            MC_Free_Item( &perfSeriesSeqID );
            return ( MERGE_FAILURE );
        }

		if(++i != p_patient->acqImageRefs.end())
		{
			MC_Free_Item( &perfSeriesSeqID );
		    if ( status != MC_NORMAL_COMPLETION )
			{
				Message( MWARNING, MLoverall | toDeveloper,
					     "setNSETRQ: line %d status %d. Failed to free the performed series sequence item.",
						 __LINE__, status );
				return ( MERGE_FAILURE );
			}

			/*
		    ** Now we re-open the item
			*/
	        status = MC_Open_Item( &perfSeriesSeqID,
		                           "PERFORMED_SERIES" );
			if ( status != MC_NORMAL_COMPLETION )
	        {
		        Message( MWARNING, MLoverall | toDeveloper,
			             "setNSETRQ: line %d status %d. Failed to open the performed series sequence item.",
				         __LINE__, status );
		        return ( MERGE_FAILURE );
			}
	        /*
		    ** And place reference to the sequence item in its respective place.
			*/
	        status = MC_Set_Next_Value_From_Int( p_messageID, 
		                                         MC_ATT_PERFORMED_SERIES_SEQUENCE,
			                                     perfSeriesSeqID );
			if ( status != MC_NORMAL_COMPLETION )
	        {
		        /*
			    ** Since we can't place the item into the message, we need to free
				** the item before we return with an error.
	            */
		        Message( MWARNING, MLoverall | toDeveloper,
			             "setNSETRQ: line %d status %d. Failed to set the performed series sequence item.",
				         __LINE__, status );
				MC_Free_Item( &perfSeriesSeqID );
				return ( MERGE_FAILURE );
			}
		}
    }

	MC_Free_Item( &perfSeriesSeqID );

	if ( strlen(p_patient->procedureCodeValue)>0 &&
		 strlen(p_patient->procedureCodingScheme)>0 )
	{
		/*
		** Now we open the Procedure Code Sequence item (0008,1032), 
		** that is part of the main message.
		*/
		status = MC_Open_Item( &procedureCodeSeqID,
			                   "PROCEDURE_CODE" );
		if ( status != MC_NORMAL_COMPLETION )
		{
			Message( MWARNING, MLoverall | toDeveloper,
				     "setNSETRQ: line %d status %d. Failed to open the Procedure Code Sequence item.",
					 __LINE__, status );
			return ( MERGE_FAILURE );
		}
		/*
		** And place reference to the sequence item (0008,1032) in its respective place.
		*/
		status = MC_Set_Value_From_Int( p_messageID, 
			                            MC_ATT_PROCEDURE_CODE_SEQUENCE,
				                        procedureCodeSeqID );
		if ( status != MC_NORMAL_COMPLETION )
		{
			/*
			** Since we can't place the item into the message, we need to free
			** the item before we return with an error.
			*/
			Message( MWARNING, MLoverall | toDeveloper,
				     "setNSETRQ: line %d status %d. Failed to set the Procedure Code Sequence item.",
					 __LINE__, status );
			MC_Free_Item( &procedureCodeSeqID );
			return ( MERGE_FAILURE );
		}

		if ( setStringValue( procedureCodeSeqID, MC_ATT_CODE_VALUE,
			                 p_patient->procedureCodeValue, "setNSETRQ" ) != MERGE_SUCCESS )
		{		
			Message( MWARNING, MLoverall | toDeveloper,
				     "setNSETRQ: line %d status %d. Failed to setNullOrStringValue for procedureCodeValue: \"%s\"", 
					 __LINE__, status, p_patient->procedureCodeValue );
			MC_Free_Item( &procedureCodeSeqID );
			return ( MERGE_FAILURE );
		}

		if ( setStringValue( procedureCodeSeqID, MC_ATT_CODING_SCHEME_DESIGNATOR,
		                     p_patient->procedureCodingScheme, "setNSETRQ" ) != MERGE_SUCCESS )
		{		
		    Message( MWARNING, MLoverall | toDeveloper,
			         "setNSETRQ: line %d status %d. Failed to setNullOrStringValue for procedureCodingScheme: \"%s\"", 
					 __LINE__, status, p_patient->procedureCodingScheme );
		    MC_Free_Item( &procedureCodeSeqID );
			return ( MERGE_FAILURE );
		}

		if ( strlen(p_patient->procedureCodeMeaning)>0 )
		{
			if ( setStringValue( procedureCodeSeqID, MC_ATT_CODE_MEANING,
				                 p_patient->procedureCodeMeaning, "setNSETRQ" ) != MERGE_SUCCESS )
			{		
				Message( MWARNING, MLoverall | toDeveloper,
						 "setNSETRQ: line %d status %d. Failed to setNullOrStringValue for procedureCodeMeaning: \"%s\"", 
						 __LINE__, status, p_patient->procedureCodeMeaning );
				MC_Free_Item( &procedureCodeSeqID );
				return ( MERGE_FAILURE );
			}
		}

	    MC_Free_Item( &procedureCodeSeqID );
	}

	if ( strlen(p_patient->performedProtocolCodeValue)>0 &&
		 strlen(p_patient->performedProtocolCodingScheme)>0 )
	{
		/*
		** Now we open the Performed Protocol Code Sequence item (0040,0260), 
		** that is part of the main message.
		*/
	    status = MC_Open_Item( &perfProtocolCodeSeqID,
		                       "PERFORMED_PROTOCOL_CODE" );
	    if ( status != MC_NORMAL_COMPLETION )
		{
			Message( MWARNING, MLoverall | toDeveloper,
				     "setNSETRQ: line %d status %d. Failed to open the Performed Protocol Code Sequence item.",
					 __LINE__, status );
			return ( MERGE_FAILURE );
	    }
		/*
	    ** And place reference to the sequence item (0040,0260) in its respective place.
		*/
	    status = MC_Set_Value_From_Int( p_messageID, 
		                                MC_ATT_PERFORMED_ACTION_ITEM_SEQUENCE,
			                            perfProtocolCodeSeqID );
	    if ( status != MC_NORMAL_COMPLETION )
		{
			/*
			** Since we can't place the item into the message, we need to free
			** the item before we return with an error.
			*/
			Message( MWARNING, MLoverall | toDeveloper,
				     "setNSETRQ: line %d status %d. Failed to set the Performed Protocol Code Sequence item.",
					 __LINE__, status );
			MC_Free_Item( &perfProtocolCodeSeqID );
			return ( MERGE_FAILURE );
		}

		if ( setStringValue( perfProtocolCodeSeqID, MC_ATT_CODE_VALUE,
			                 p_patient->performedProtocolCodeValue, "setNSETRQ" ) != MERGE_SUCCESS )
		{		
			Message( MWARNING, MLoverall | toDeveloper,
				     "setNSETRQ: line %d status %d. Failed to setNullOrStringValue for performedProtocolCodeValue: \"%s\"", 
					 __LINE__, status, p_patient->performedProtocolCodeValue );
		    MC_Free_Item( &perfProtocolCodeSeqID );
			return ( MERGE_FAILURE );
		}

		if ( setStringValue( perfProtocolCodeSeqID, MC_ATT_CODING_SCHEME_DESIGNATOR,
			                 p_patient->performedProtocolCodingScheme, "setNSETRQ" ) != MERGE_SUCCESS )
		{		
			Message( MWARNING, MLoverall | toDeveloper,
				     "setNSETRQ: line %d status %d. Failed to setNullOrStringValue for performedProtocolCodingScheme: \"%s\"", 
					 __LINE__, status, p_patient->performedProtocolCodingScheme );
			MC_Free_Item( &perfProtocolCodeSeqID );
		    return ( MERGE_FAILURE );
		}

		if ( strlen(p_patient->performedProtocolCodeMeaning)>0 )
		{
			if ( setStringValue( perfProtocolCodeSeqID, MC_ATT_CODE_MEANING,
					             p_patient->performedProtocolCodeMeaning, "setNSETRQ" ) != MERGE_SUCCESS )
			{		
				Message( MWARNING, MLoverall | toDeveloper,
						 "setNSETRQ: line %d status %d. Failed to setNullOrStringValue for performedProtocolCodeMeaning: \"%s\"", 
						 __LINE__, status, p_patient->performedProtocolCodeMeaning );
				MC_Free_Item( &perfProtocolCodeSeqID );
				return ( MERGE_FAILURE );
			}
		}

	    MC_Free_Item( &perfProtocolCodeSeqID );
	}

    return( MERGE_SUCCESS );
}

void strllcpy (char * dest, const char * src, size_t size)
{
	memset(dest, 0, size);
	strncpy(dest, src, size);
	dest[size-1] = '\0';
}
