/*
 * file:	worklistutils.cc
 * purpose:	Implementation of WorklistUtils class method
 *
 * revision history:
 *   Jiantao Huang		    initial version
*/

#include <stdlib.h>
#include <ctype.h>
#include <iostream>
#include <iomanip> 
#include <fstream> 

#include "worklistutils.h"
#ifdef REQUIRES_MERGE_LEAKFIX 
#include "pic.h"
#endif

/*****************************************************************************
**
** NAME
**    PerformInitialization
**
** SYNOPSIS
**    Does the library initialization, registers this as an application,
**    and sets up some initial program values
**
**    This method is moved from class WorklistUtils here
**    because this method should only be called once per
**    application. Not per class instantiation. Otherwise,
**    get error MC_LIBRARY_ALREADY_INITIALIZED
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
	Message( MWARNING, MLoverall | toDeveloper, "PerformInitialization: line %d status %d. Unable to initialize the library", __LINE__, status);
        return( MERGE_FAILURE );
    }

    /*
    ** Once we've successfully initialized the library, then we must
    ** register this application with it.  "p_localAppTitle" is either
    ** defaulted to MERGE_WORK_SCU, or entered on the command line.
    */
    status = MC_Register_Application( p_applicationID, p_localAppTitle );

    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "PerformInitialization: line %d status %d. Unable to register this application (calling AE Title = %s) with the library", __LINE__, status,  p_localAppTitle);
        return( MERGE_FAILURE );
    }

    return( MERGE_SUCCESS );
}

WorklistUtils::WorklistUtils(const char* p_destAppTitle,
                             int p_portNumber,
                             const char* p_remoteHost,
                             int p_timeout,
                             int p_threshold
                             )
{
	strcpy(_destAppTitle, p_destAppTitle);
	_portNumber = p_portNumber;
	strcpy(_remoteHost, p_remoteHost);
	_timeout = p_timeout;
	_threshold = p_threshold;

	getAttributeFilter();
}

WorklistUtils::~WorklistUtils() throw ()
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
int WorklistUtils::openAssociation(int  p_applicationID) throw (WorklistUtils::WorklistUtilsException)
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
                                  attemptHost, const_cast<char*>("Worklist_Service_List") );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "openAssociation: line %d status %d. Unable to create an association with %s.", __LINE__, status, _destAppTitle );
        throw ( WORKLISTUTILS_NETWORK );
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
**    char*             p_value     - the value to set
**    char*             p_function  - a function name to reference
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
WorklistUtils::setNullOrStringValue( 
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
            return ( MERGE_FAILURE );
        }
    }
    else
    {
        status = MC_Set_Value_To_NULL( p_messageID,
                                       p_tag );
        if ( status != MC_NORMAL_COMPLETION )
        {
            MC_Get_Tag_Info(p_tag, tagDescription, sizeof(tagDescription));
            Message( MWARNING, MLoverall | toDeveloper, "setNullOrStringValue: line %d, status %d. Failed to NULL \"%s\" for p_function \"%s\"", __LINE__, status, tagDescription, p_function );
            return ( MERGE_FAILURE );
        }
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
**    char*             p_value     - the value to set
**    char*             p_function  - the calling function's name
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
WorklistUtils::setStringValue( 
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
**    sendWorklistQuery
**
** SYNOPSIS
**    void sendWorklistQuery
**
** ARGUMENTS
**    int  p_associationID - id of the association
**    QueryPara queryPara - the query parameters
**
** DESCRIPTION
**    Opens a message, fills in some of the "required" fields, and then
**    sends the message off to the service class provider.
**
** RETURNS
**    None
**
** SEE ALSO
**    
*****************************************************************************/
void
WorklistUtils::sendWorklistQuery( int p_associationID, QueryPara queryPara )
                                throw (WorklistUtils::WorklistUtilsException)
{
    MC_STATUS    status;
    int          messageID;
    int          procedureStepItemID;
    int          procedureCodeItemID;
//    int          referencedStudyComponentItemNum;
    int          referencedStudyItemNum;

    /*
    ** Since we've gotten an association with the remote application
    ** (the server), and we know what the user wants to do,  we are able
    ** to perform a C-FIND.  But first, we have to setup the message that
    ** we'll be passing to the server.  Note:  the WORK_SCP is only
    ** capable of a MODALITY_WORKLIST_FIND therefore, our message will be
    ** a find.  Setting up the message usually consists of two steps:
    ** opening the message, and then setting the message's tag values.
    */
    status = MC_Open_Message( &messageID, "MODALITY_WORKLIST_FIND",
                              C_FIND_RQ );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d, status %d. Unable to open a message for MODALITY_WORKLIST_FIND", __LINE__, status );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    /*
    ** Some of the values (AE title, start date, modality, performing physician)
    ** that we want to query on are part of Scheduled Procedure Step Sequence
    ** and the rest (accessionNumber, referringPhysician, patientName,
    ** patientID) are part of Requested Procedure Code Sequence within the FIND.
    ** Therefore, we attempt to open items for the former group. 
    */
    status = MC_Open_Item( &procedureStepItemID,
                           "SCHEDULED_PROCEDURE_STEP" );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d, status %d. Unable to open an item for SCHEDULED_PROCEDURE_STEP", __LINE__, status );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    /*
    ** Now we have a message and we have an item.  The item is the sequence
    ** of procedures.  We need to connect this item to the message.  The
    ** following places the id of the item into the message in place of the
    ** procedure step sequence.
    */
    status = MC_Set_Value_From_Int( messageID,
                                   MC_ATT_SCHEDULED_PROCEDURE_STEP_SEQUENCE,
                                   procedureStepItemID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d, status %d\n"
                  "Unable to set value for MSG_ID:  %d\n"
				  "TAG (MC_ATT_SCHEDULED_PROCEDURE_STEP_SEQUENCE):  %d\n"
                  "                     TAG VALUE:  %d\n",
                  __LINE__, status, messageID,
                  MC_ATT_SCHEDULED_PROCEDURE_STEP_SEQUENCE,
                  procedureStepItemID );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    /*
    ** In order to setup the message, we use the MC_Set_Value... functions.
    ** We also include some fields that are part of the IHE requirements.
    ** We set those values to NULL, indicitating that the SCP should give
    ** them to us if it can.
    */

    if ( setNullOrStringValue( procedureStepItemID, 
                        MC_ATT_SCHEDULED_STATION_AE_TITLE,
                        queryPara.localAeTitle, 
                        "sendWorklistQuery" ) != MERGE_SUCCESS ) 
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d, status %d\n"
                  "Unable to set value for MSG_ID:  %d\n"
                  "          TAG (Local AE Title):  %d\n"
                  "                     TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_SCHEDULED_STATION_AE_TITLE,
                  queryPara.localAeTitle );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    if ( setNullOrStringValue( procedureStepItemID, 
                        MC_ATT_SCHEDULED_PROCEDURE_STEP_START_DATE,
                        queryPara.scheduledProcStepStartDate, 
                        "sendWorklistQuery" ) != MERGE_SUCCESS ) 
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d, status %d\n"
                  "Unable to set value for MSG_ID:  %d\n"
                  "          TAG (SPS Start Date):  %d\n"
                  "                     TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_SCHEDULED_PROCEDURE_STEP_START_DATE,
                  queryPara.scheduledProcStepStartDate );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    if ( setNullOrStringValue( procedureStepItemID, 
                        MC_ATT_SCHEDULED_PROCEDURE_STEP_START_TIME,
                        queryPara.scheduledProcStepStartTime, 
                        "sendWorklistQuery" ) != MERGE_SUCCESS ) 
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d, status %d\n"
                  "Unable to set value for MSG_ID:  %d\n"
                  "          TAG (SPS Start Time):  %d\n"
                  "                     TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_SCHEDULED_PROCEDURE_STEP_START_TIME,
                  queryPara.scheduledProcStepStartTime );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    if ( setNullOrStringValue( procedureStepItemID, 
                               MC_ATT_MODALITY, 
                               queryPara.modality, 
                               "sendWorklistQuery" ) != MERGE_SUCCESS ) 
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d, status %d\n"
                  "Unable to set value for MSG_ID:  %d\n"
                  "                TAG (Modality):  %d\n"
                  "                     TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_MODALITY,
                  queryPara.modality );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    /*
    ** We allow query by physician's name. Set it properly here if supplied.
    */
    if ( setNullOrStringValue( procedureStepItemID, 
                        MC_ATT_SCHEDULED_PERFORMING_PHYSICIANS_NAME,
                        queryPara.scheduledPerformPhysicianName, 
                        "sendWorklistQuery" ) != MERGE_SUCCESS ) 
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d, status %d\n"
                  "Unable to set value for MSG_ID:  %d\n"
                  "    TAG (PerformPhysicianName):  %d\n"
                  "                     TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_SCHEDULED_PERFORMING_PHYSICIANS_NAME,
                  queryPara.scheduledPerformPhysicianName );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    /*
    ** We will default those that the server is going to look at,
    ** lest we get invalid tag messages.
    */
    status = MC_Set_Value_To_NULL( procedureStepItemID,
                                   MC_ATT_SCHEDULED_PROCEDURE_STEP_DESCRIPTION );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "  Unable to set NULL value for MSG_ID:  %d\n"
                  "     TAG (scheduledProcedureStepDesc):  %d\n"
                  "                            TAG VALUE:  %s\n",
                  __LINE__, status, procedureStepItemID,
                  MC_ATT_SCHEDULED_PROCEDURE_STEP_DESCRIPTION,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( procedureStepItemID,
                                   MC_ATT_SCHEDULED_STATION_NAME );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "       TAG (SCHEDULED_STATION_NAME):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, procedureStepItemID,
                  MC_ATT_SCHEDULED_STATION_NAME,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( procedureStepItemID,
                                   MC_ATT_SCHEDULED_PROCEDURE_STEP_ID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "     TAG (scheduledProcedureStepID):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, procedureStepItemID,
                  MC_ATT_SCHEDULED_PROCEDURE_STEP_ID,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    /*
    ** Scheduled Protocol Code Meaning (0008,0104) is
    ** part of Scheduled Protocol Code Sequence (0040,0008) group.
    ** Therefore, we attempt to open an item for this group. 
    */
    status = MC_Open_Item( &procedureCodeItemID,
                           "SCHEDULED_PROTOCOL_CODE" );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d, status %d. Unable to open an item for SCHEDULED_PROTOCOL_CODE", __LINE__, status );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    /*
    ** Now we have a message and we have an item.  The item is the sequence
    ** of procedure code.  We need to connect this item to the message.  The
    ** following places the id of the item into the message in place of the
    ** procedure code sequence.
    */
    status = MC_Set_Value_From_Int( procedureStepItemID,
                                    MC_ATT_SCHEDULED_ACTION_ITEM_CODE_SEQUENCE,
                                    procedureCodeItemID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d, status %d\n"
                  "Unable to set value for MSG_ID:  %d\n"
				  "TAG (MC_ATT_SCHEDULED_ACTION_ITEM_CODE_SEQUENCE):  %d\n"
                  "                     TAG VALUE:  %d\n",
                  __LINE__, status, messageID,
                  MC_ATT_SCHEDULED_ACTION_ITEM_CODE_SEQUENCE,
                  procedureCodeItemID );
        MC_Free_Item( &procedureCodeItemID );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( procedureCodeItemID,
                                   MC_ATT_CODE_VALUE );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "            TAG (MC_ATT_CODE_VALUE):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, procedureCodeItemID,
                  MC_ATT_CODE_VALUE,
                  "NULL" );
        MC_Free_Item( &procedureCodeItemID );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( procedureCodeItemID,
                                   MC_ATT_CODING_SCHEME_DESIGNATOR );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "TAG (MC_ATT_CODING_SCHEME_DESIGNATOR):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, procedureCodeItemID,
                  MC_ATT_CODING_SCHEME_DESIGNATOR,
                  "NULL" );
        MC_Free_Item( &procedureCodeItemID );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

	status = MC_Set_Value_To_NULL( procedureCodeItemID,
                                   MC_ATT_CODE_MEANING );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "          TAG (MC_ATT_CODE_MEANING):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, procedureCodeItemID,
                  MC_ATT_CODE_MEANING,
                  "NULL" );
        MC_Free_Item( &procedureCodeItemID );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    MC_Free_Item( &procedureCodeItemID );

#ifdef MPPS_ATTR
	/*
    ** Referenced Component SOP Class UID (0008,1150) is
    ** part of the Referenced Study Component Sequence (0008,1111).
    ** Therefore, we attempt to open an item for this group. 
    */
    status = MC_Open_Item( &referencedStudyComponentItemNum,
                           "REFERENCED_PERFORMED_PROCEDURE_STEP_SEQUENCE" );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d, status %d. Unable to open an item for REFERENCED_PERFORMED_PROCEDURE_STEP_SEQUENCE", __LINE__, status );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    /*
    ** Now we need to connect them.
    */
    status = MC_Set_Value_From_Int( procedureStepItemID,
                                    MC_ATT_REFERENCED_STUDY_COMPONENT_SEQUENCE,
                                    referencedStudyComponentItemNum );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d, status %d\n"
                  "Unable to set value for MSG_ID:  %d\n"
				  "TAG (MC_ATT_REFERENCED_STUDY_COMPONENT_SEQUENCE):  %d\n"
                  "                     TAG VALUE:  %d\n",
                  __LINE__, status, procedureStepItemID,
                  MC_ATT_REFERENCED_STUDY_COMPONENT_SEQUENCE,
                  referencedStudyComponentItemNum );
        MC_Free_Item( &referencedStudyComponentItemNum );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( referencedStudyComponentItemNum,
                                   MC_ATT_REFERENCED_SOP_CLASS_UID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "TAG (MC_ATT_REFERENCED_SOP_CLASS_UID):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, referencedStudyComponentItemNum,
                  MC_ATT_REFERENCED_SOP_CLASS_UID,
                  "NULL" );
        MC_Free_Item( &referencedStudyComponentItemNum );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( referencedStudyComponentItemNum,
                                   MC_ATT_REFERENCED_SOP_INSTANCE_UID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "TAG (MC_ATT_REFERENCED_SOP_INSTANCE_UID):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, referencedStudyComponentItemNum,
                  MC_ATT_REFERENCED_SOP_INSTANCE_UID,
                  "NULL" );
        MC_Free_Item( &referencedStudyComponentItemNum );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    MC_Free_Item( &referencedStudyComponentItemNum );
#endif

	/*
    ** Again, if the requested procedure id isn't set, we will set it
    ** to the NULL value.
    */
    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_REQUESTED_PROCEDURE_ID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "       TAG (REQUESTED_PROCEDURE_ID):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_REQUESTED_PROCEDURE_ID,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    /*
    ** Requested procedure description...
    */
    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_REQUESTED_PROCEDURE_DESCRIPTION );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "  Unable to set NULL value for MSG_ID:  %d\n"
                  "TAG (REQUESTED_PROCEDURE_DESCRIPTION):  %d\n"
                  "                            TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_REQUESTED_PROCEDURE_DESCRIPTION,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    /*
    ** Requested Procedure Code (0008,0104) is
    ** part of Requested Procedure Code Sequence (0032,1064) group. Therefore,
    ** we attempt to open an item for this group. 
    */
    status = MC_Open_Item( &procedureCodeItemID,
                           "REQUESTED_PROCEDURE_CODE" );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d, status %d. Unable to open an item for REQUESTED_PROCEDURE_CODE", __LINE__, status );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    /*
    ** Now we have a message and we have an item.  The item is the sequence
    ** of procedure code.  We need to connect this item to the message.  The
    ** following places the id of the item into the message in place of the
    ** procedure code sequence.
    */
    status = MC_Set_Value_From_Int( messageID,
                                    MC_ATT_REQUESTED_PROCEDURE_CODE_SEQUENCE,
                                    procedureCodeItemID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d, status %d\n"
                  "Unable to set value for MSG_ID:  %d\n"
				  "TAG (MC_ATT_REQUESTED_PROCEDURE_CODE_SEQUENCE):  %d\n"
                  "                     TAG VALUE:  %d\n",
                  __LINE__, status, messageID,
                  MC_ATT_REQUESTED_PROCEDURE_CODE_SEQUENCE,
                  procedureCodeItemID );
        MC_Free_Item( &procedureCodeItemID );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( procedureCodeItemID,
                                   MC_ATT_CODE_VALUE );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "            TAG (MC_ATT_CODE_VALUE):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, procedureCodeItemID,
                  MC_ATT_CODE_VALUE,
                  "NULL" );
        MC_Free_Item( &procedureCodeItemID );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( procedureCodeItemID,
                                   MC_ATT_CODING_SCHEME_DESIGNATOR );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "TAG (MC_ATT_CODING_SCHEME_DESIGNATOR):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, procedureCodeItemID,
                  MC_ATT_CODING_SCHEME_DESIGNATOR,
                  "NULL" );
        MC_Free_Item( &procedureCodeItemID );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

	status = MC_Set_Value_To_NULL( procedureCodeItemID,
                                   MC_ATT_CODE_MEANING );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "          TAG (MC_ATT_CODE_MEANING):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, procedureCodeItemID,
                  MC_ATT_CODE_MEANING,
                  "NULL" );
        MC_Free_Item( &procedureCodeItemID );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

   MC_Free_Item( &procedureCodeItemID );

    /*
    ** Again, if the study instance uid isn't set, we will set it
    ** to the NULL value.
    */
    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_STUDY_INSTANCE_UID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "           TAG (STUDY_INSTANCE_UID):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_STUDY_INSTANCE_UID,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    if ( setNullOrStringValue( messageID, 
                               MC_ATT_ACCESSION_NUMBER,
                               queryPara.accessionNumber,
                               "sendWorklistQuery" ) != MERGE_SUCCESS ) 
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "             TAG (ACCESSION_NUMBER):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_ACCESSION_NUMBER,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    if ( setNullOrStringValue( messageID, 
                               MC_ATT_REQUESTING_PHYSICIAN,
                               queryPara.requestingPhysician,
                               "sendWorklistQuery" ) != MERGE_SUCCESS ) 
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "         TAG (REQUESTING_PHYSICIAN):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_REQUESTING_PHYSICIAN,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    if ( setNullOrStringValue( messageID, 
                               MC_ATT_REFERRING_PHYSICIANS_NAME,
                               queryPara.referringPhysician,
                               "sendWorklistQuery" ) != MERGE_SUCCESS ) 
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "    TAG (REFERRING_PHYSICIANS_NAME):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_REFERRING_PHYSICIANS_NAME,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    /*
    ** Again, if the patient name isn't set, we will set it
    ** to the NULL value.
    */
    if ( setNullOrStringValue( messageID, 
                               MC_ATT_PATIENTS_NAME,
                               queryPara.patientName, 
                               "sendWorklistQuery" ) != MERGE_SUCCESS ) 
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set value for MSG_ID:  %d\n"
                  "             TAG (patientName):  %d\n"
                  "                     TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_PATIENTS_NAME,
                  queryPara.patientName );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    /*
    ** Again, if the patient id isn't set, we will set it
    ** to the NULL value.
    */
    if ( setNullOrStringValue( messageID, 
                               MC_ATT_PATIENT_ID,
                               queryPara.patientID, 
                               "sendWorklistQuery" ) != MERGE_SUCCESS ) 
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set value for MSG_ID:  %d\n"
                  "               TAG (patientID):  %d\n"
                  "                     TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_PATIENT_ID,
                  queryPara.patientID );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_PATIENTS_BIRTH_DATE );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "          TAG (PATIENTS_BIRTH_DATE):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_PATIENTS_BIRTH_DATE,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_PATIENTS_SIZE );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "         TAG (MC_ATT_PATIENTS_SIZE):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_PATIENTS_SIZE,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_PATIENTS_WEIGHT );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "       TAG (MC_ATT_PATIENTS_WEIGHT):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_PATIENTS_WEIGHT,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_PATIENTS_SEX );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "                 TAG (PATIENTS_SEX):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_PATIENTS_SEX,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_PATIENT_STATE );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "         TAG (MC_ATT_PATIENT_STATE):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_PATIENT_STATE,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_PREGNANCY_STATUS );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "      TAG (MC_ATT_PREGNANCY_STATUS):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_PREGNANCY_STATUS,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_MEDICAL_ALERTS );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "        TAG (MC_ATT_MEDICAL_ALERTS):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_MEDICAL_ALERTS,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_CONTRAST_ALLERGIES );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "    TAG (MC_ATT_CONTRAST_ALLERGIES):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_CONTRAST_ALLERGIES,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_SPECIAL_NEEDS );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "         TAG (MC_ATT_SPECIAL_NEEDS):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_SPECIAL_NEEDS,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_OTHER_PATIENT_IDS );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "     TAG (MC_ATT_OTHER_PATIENT_IDS):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_OTHER_PATIENT_IDS,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_NAMES_OF_INTENDED_RECIPIENTS_OF_RESULTS );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "                 Unable to set NULL value for MSG_ID:  %d\n"
                  "TAG (MC_ATT_NAMES_OF_INTENDED_RECIPIENTS_OF_RESULTS):  %d\n"
                  "                                           TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_NAMES_OF_INTENDED_RECIPIENTS_OF_RESULTS,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_IMAGING_SERVICE_REQUEST_COMMENTS );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "          Unable to set NULL value for MSG_ID:  %d\n"
                  "TAG (MC_ATT_IMAGING_SERVICE_REQUEST_COMMENTS):  %d\n"
                  "                                    TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_IMAGING_SERVICE_REQUEST_COMMENTS,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_REQUESTED_PROCEDURE_COMMENTS );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "      Unable to set NULL value for MSG_ID:  %d\n"
                  "TAG (MC_ATT_REQUESTED_PROCEDURE_COMMENTS):  %d\n"
                  "                                TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_REQUESTED_PROCEDURE_COMMENTS,
                  "NULL" );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

#ifdef INVALID_TAG
// Somehow it reports invalid tag "SERIES_INSTANCE_UID". This is not a worklist return key.
    int          referencedSeriesItemID;

    /*
    ** seriesInstanceUID (0020,000E) is
    ** part of Referenced Series Sequence (0008,1115) group. Therefore,
    ** we attempt to open an item for this group. 
    */
    status = MC_Open_Item( &referencedSeriesItemID,
                           "REFERENCED_SERIES" );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d, status %d. Unable to open an item for REFERENCED_SERIES", __LINE__, status );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    /*
    ** Now we have a message and we have an item.  The item is the sequence
    ** of Referenced Series.  We need to connect this item to the message.  The
    ** following places the id of the item into the message in place of the
    ** Referenced Series sequence.
    */
    status = MC_Set_Value_From_Int( messageID,
                                    MC_ATT_REFERENCED_SERIES_SEQUENCE,
                                    referencedSeriesItemID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d, status %d\n"
                  "Unable to set value for MSG_ID:  %d\n"
				  "TAG (MC_ATT_RT_REFERENCED_SERIES_SEQUENCE):  %d\n"
                  "                     TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_RT_REFERENCED_SERIES_SEQUENCE,
                  referencedSeriesItemID );
        MC_Free_Item( &referencedSeriesItemID );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

	status = MC_Set_Value_To_NULL( referencedSeriesItemID,
                                   MC_ATT_SERIES_INSTANCE_UID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "   TAG (MC_ATT_SERIES_INSTANCE_UID):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_SERIES_INSTANCE_UID,
                  "NULL" );
        MC_Free_Item( &referencedSeriesItemID );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }
#endif

    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_REASON_FOR_THE_REQUESTED_PROCEDURE );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "            Unable to set NULL value for MSG_ID:  %d\n"
                  "TAG (MC_ATT_REASON_FOR_THE_REQUESTED_PROCEDURE):  %d\n"
                  "                                      TAG VALUE:  %s\n",
                  __LINE__, status, messageID,
                  MC_ATT_REASON_FOR_THE_REQUESTED_PROCEDURE,
                  "NULL" );
//        MC_Free_Item( &referencedSeriesItemID );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    /*
    ** Referenced SOP Class UID (0008,1150) is
    ** part of the Referenced Study Sequence (0008,1110).
    ** Therefore, we attempt to open an item for this group. 
    */
    status = MC_Open_Item( &referencedStudyItemNum,
                           "REFERENCED_STUDY_SEQUENCE" );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d, status %d. Unable to open an item for REFERENCED_STUDY_SEQUENCE", __LINE__, status );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    /*
    ** Now we need to connect them.
    */
    status = MC_Set_Value_From_Int( messageID,
                                    MC_ATT_REFERENCED_STUDY_SEQUENCE,
                                    referencedStudyItemNum );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d, status %d\n"
                  "Unable to set value for MSG_ID:  %d\n"
				  "TAG (MC_ATT_REFERENCED_STUDY_SEQUENCE):  %d\n"
                  "                     TAG VALUE:  %d\n",
                  __LINE__, status, messageID,
                  MC_ATT_REFERENCED_STUDY_SEQUENCE,
                  referencedStudyItemNum );
        MC_Free_Item( &referencedStudyItemNum );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( referencedStudyItemNum,
                                   MC_ATT_REFERENCED_SOP_CLASS_UID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "TAG (MC_ATT_REFERENCED_SOP_CLASS_UID):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, referencedStudyItemNum,
                  MC_ATT_REFERENCED_SOP_CLASS_UID,
                  "NULL" );
        MC_Free_Item( &referencedStudyItemNum );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    status = MC_Set_Value_To_NULL( referencedStudyItemNum,
                                   MC_ATT_REFERENCED_SOP_INSTANCE_UID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendWorklistQuery: line %d status %d.\n"
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "TAG (MC_ATT_REFERENCED_SOP_INSTANCE_UID):  %d\n"
                  "                          TAG VALUE:  %s\n",
                  __LINE__, status, referencedStudyItemNum,
                  MC_ATT_REFERENCED_SOP_INSTANCE_UID,
                  "NULL" );
        MC_Free_Item( &referencedStudyItemNum );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_SOFTWARE );
    }

    MC_Free_Item( &referencedStudyItemNum );

	/*
     * Validate and list the message
     */
    ValMessage      (messageID, "C-FIND-RQ");
    WriteToListFile (messageID, "cfindrq.msg");

    /*
    ** Once the message has been built, we are then able to perform the
    ** C-FIND on it.
    */
    status = MC_Send_Request_Message( p_associationID, messageID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "SetAndSendWorklistMessage: line %d, status %d. Unable to send the request message to the SCP.", __LINE__, status );
        MC_Free_Message( &messageID );
        throw ( WORKLISTUTILS_NETWORK );
    }

#ifdef REQUIRES_MERGE_LEAKFIX 
	PegasusLibThreadTerm();
#endif

	/*
    ** After sending the message, we free it.
    */
    status = MC_Free_Message( &messageID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "SetAndSendWorklistMessage: line %d, status %d. Unable to free the request message.", __LINE__, status );
        throw ( WORKLISTUTILS_MEMORY );
    }
    
} /* End of sendWorklistQuery */


/*****************************************************************************
**
** NAME
**    processWorklistReplyMsg
**
** SYNOPSIS
**    void processWorklistReplyMsg
**
** ARGUMENTS
**    int   p_associationID - the association
**    LIST *p_list          - a pointer to the worklist's list
**
** DESCRIPTION
**    Waits for a reply message(s) from the service class provider.
**    Once the server sends a message, we then want to process the
**    message.
**
** RETURNS
**    None
**
** SEE ALSO
**    
*****************************************************************************/
void
WorklistUtils::processWorklistReplyMsg(
                           int   p_associationID,
                           LIST *p_list
                       )
                       throw (WorklistUtils::WorklistUtilsException)
{
    MC_STATUS    status;
    char         *serviceName;         /* This is a pointer to the name of */
                                       /* the service associated with the  */
                                       /*  message ID.                     */
                                       /* It is used in the return msg.    */
    MC_COMMAND   command;              /* The command associated with the  */
                                       /* return message.                  */
    unsigned int response;             /* The response value from the SCP  */
    int   responseMessageID;           /* The response message from the    */
                                       /* SCP                              */
    WorkPatientRec patientRec;         /* The data from the patient        */
    int   procedureStepItemNum;        /* The item number that pertains to */
                                       /* the sequence of procedure steps  */
    int   procedureCodeItemNum;        /* The item number that pertains to */
                                       /* the sequence of procedure code   */
    int   messageCount = 0;            /* The number of responses from SCP */
    bool  alreadySent = false;
    unsigned int tempPregnancyStatus;
    const char* strPregStat[5] = {"", "not pregnant", "possibly pregnant", "definitely pregnant", "unknown"};
	int   errorReportTimesCount = 0;
//	int   referencedStudyComponentItemNum;
	int   referencedStudyItemNum;

    /*
    ** Until finished,  we'll loop, looking for messages from the server.
    */
    while( 1 )
    {
		errorReportTimesCount++;

        /*
        ** Read a message from the formed association, timing out in
        ** _timeout seconds.
        */
        status = MC_Read_Message( p_associationID, _timeout,
                                  &responseMessageID,
                                  &serviceName, &command );
        if ( status != MC_NORMAL_COMPLETION )
        {
            Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error reading the message from the SCP.", __LINE__, status );
            throw( WORKLISTUTILS_SOFTWARE );
        }

        /*
        ** Once we've read the message, then we check the operation to make
        ** sure that we haven't completed the reading...
        */
        status = MC_Get_Value_To_UInt( responseMessageID, MC_ATT_STATUS,
                                       &response );

        if ( status != MC_NORMAL_COMPLETION )
        {
            MC_Free_Message( &responseMessageID );
            Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg : line %d, status %d. Error checking the response from the SCP.", __LINE__, status );
            throw( WORKLISTUTILS_SOFTWARE );
        }

        if ( response == C_FIND_SUCCESS )
        {
            /*
            ** Since we've gotten a success, we've read the last message
            ** from the server.
            */
            status = MC_Free_Message( &responseMessageID );
            if ( status != MC_NORMAL_COMPLETION )
            {
                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error freeing message: %d",
                          __LINE__, status, responseMessageID );
                throw( WORKLISTUTILS_MEMORY );
            }
            break;
        }
        /*
        ** For this case, we are dealing with a real message and the
        ** data contained within it.
        */
        else if ( ( response == C_FIND_PENDING_NO_OPTIONAL_KEY_SUPPORT ) ||
                  ( response == C_FIND_PENDING ) )
        {
            /*
            ** Clean the buffer to store the patient record
            */
            memset((void*)&patientRec, 0, sizeof(patientRec));

            /*
            ** Here is where we actually grab the data from the toolkit's
            ** data strctures and attempt to display it to the user.
            */

            /*
            ** Since some of the responses are contained within a sequence,
            ** we will attempt to open that sequence as an item:
            */
            status = MC_Get_Value_To_Int( responseMessageID,
                                          MC_ATT_SCHEDULED_PROCEDURE_STEP_SEQUENCE,
                                          &procedureStepItemNum );
            if ( status == MC_NULL_VALUE )
            {
                if (errorReportTimesCount < 2)
                    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for SCHEDULED_PROCEDURE_STEP_SEQUENCE and `schuduled' attributes could not be extracted.");
			}
            else if ( status != MC_NORMAL_COMPLETION )
            {
                if (errorReportTimesCount < 2)
                    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `scheduled procedure step sequence id'.", __LINE__, status );
                MC_Free_Message( &responseMessageID );
                throw( WORKLISTUTILS_SOFTWARE );
            }
            else
            {
                /*
                ** Get the Scheduled Station AE Title (0040,0001).
                */
				if ( !_attributeBlocked[scheduledStationLocalAETitle] )
				{
                    status = MC_Get_Value_To_String( procedureStepItemNum,
                                                     MC_ATT_SCHEDULED_STATION_AE_TITLE,
                                                     sizeof( patientRec.scheduledStationLocalAETitle ),
                                                     patientRec.scheduledStationLocalAETitle );
                    if ( status == MC_NULL_VALUE )
                    {
                        if (errorReportTimesCount < 2)
                            Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `scheduledStationLocalAETitle'");
				    }
                    else if (status != MC_NORMAL_COMPLETION )
                    {
                        if (errorReportTimesCount < 2)
                            Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `Scheduled Station AE Title'.", __LINE__, status );
//                        MC_Free_Message( &responseMessageID );
//                        throw( WORKLISTUTILS_SOFTWARE );
                    }
				}

                /*
                ** If we were given a start date, then fill it in. (0040,0002)
                */
				if ( !_attributeBlocked[scheduledProcedureStepStartDate] )
				{
					status = MC_Get_Value_To_String( procedureStepItemNum,
		                                            MC_ATT_SCHEDULED_PROCEDURE_STEP_START_DATE,
			                                         sizeof( patientRec.scheduledProcedureStepStartDate ),
				                                     patientRec.scheduledProcedureStepStartDate );
					if ( status == MC_NULL_VALUE )
					{
						if (errorReportTimesCount < 2)
							Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `scheduledProcedureStepStartDate'");
					}
					else if (status != MC_NORMAL_COMPLETION )
					{
						if (errorReportTimesCount < 2)
							Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `scheduled procedure step start date'.", __LINE__, status );
//						  MC_Free_Message( &responseMessageID );
//						  throw( WORKLISTUTILS_SOFTWARE );
					}
				}
          
                /*
                ** If we were given a start time, then fill it in. (0040,0003)
                */
				if ( !_attributeBlocked[scheduledProcedureStepStartTime] )
				{
	                status = MC_Get_Value_To_String( procedureStepItemNum,
		                                             MC_ATT_SCHEDULED_PROCEDURE_STEP_START_TIME,
			                                         sizeof( patientRec.scheduledProcedureStepStartTime ),
				                                     patientRec.scheduledProcedureStepStartTime );
	                if ( status == MC_NULL_VALUE )
		            {
			            if (errorReportTimesCount < 2)
				            Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `scheduledProcedureStepStartTime'");
					}
	                else if (status != MC_NORMAL_COMPLETION )
		            {
			            if (errorReportTimesCount < 2)
				          Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `scheduled procedure step start time'.", __LINE__, status );
//					      MC_Free_Message( &responseMessageID );
//						  throw( WORKLISTUTILS_SOFTWARE );
					}
				}

                /*
                **  When given a modality, fill it in. (0008,0060)
                */
				if ( !_attributeBlocked[modality] )
				{
	                status = MC_Get_Value_To_String( procedureStepItemNum,
		                                             MC_ATT_MODALITY,
			                                         sizeof( patientRec.modality ),
				                                     patientRec.modality );
					if ( status == MC_NULL_VALUE )
	                {
		                if (errorReportTimesCount < 2)
			                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for modality");
					}
					else if (status != MC_NORMAL_COMPLETION )
	                {
		                if (errorReportTimesCount < 2)
			                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `modality'.", __LINE__, status );
//				          MC_Free_Message( &responseMessageID );
//					      throw( WORKLISTUTILS_SOFTWARE );
					}
				}

                /*
                ** Now we have a performing physician's name, so fill it in. (0040,0006)
                */
				if ( !_attributeBlocked[scheduledPerformingPhysiciansName] )
				{
	                status = MC_Get_Value_To_String( procedureStepItemNum,
		                                             MC_ATT_SCHEDULED_PERFORMING_PHYSICIANS_NAME,
			                                         sizeof( patientRec.scheduledPerformingPhysiciansName ),
				                                 patientRec.scheduledPerformingPhysiciansName );
					if ( status == MC_NULL_VALUE )
	                {
		                if (errorReportTimesCount < 2)
			                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `scheduledPerformingPhysiciansName'");
					}
				    else if (status != MC_NORMAL_COMPLETION )
					{
						if (errorReportTimesCount < 2)
		                    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `performing physician's name'.", __LINE__, status );
//			              MC_Free_Message( &responseMessageID );
//				          throw( WORKLISTUTILS_SOFTWARE );
					}
				}

                /*
                ** When given a scheduled procedure step description, we want
                ** to fill that in as well. (0040,0007)
                */
				if ( !_attributeBlocked[scheduledProcedureStepDescription] )
				{
	                status = MC_Get_Value_To_String( procedureStepItemNum,
		                                             MC_ATT_SCHEDULED_PROCEDURE_STEP_DESCRIPTION,
			                                         sizeof( patientRec.scheduledProcedureStepDescription ),
				                                     patientRec.scheduledProcedureStepDescription );
	                if ( status == MC_NULL_VALUE )
		            {
			            if (errorReportTimesCount < 2)
				            Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `scheduledProcedureStepDescription'");
					}
		            else if (status != MC_NORMAL_COMPLETION )
			        {
				        if (errorReportTimesCount < 2)
					        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `scheduled procedure step description'.", __LINE__, status );
//						  MC_Free_Message( &responseMessageID );
//						  throw( WORKLISTUTILS_SOFTWARE );
				    }
				}

                /*
                ** When given a scheduled station name, we want
                ** to fill that in as well. (0040,0010)
                */
				if ( !_attributeBlocked[scheduledStationName] )
				{
	                status = MC_Get_Value_To_String( procedureStepItemNum,
		                                             MC_ATT_SCHEDULED_STATION_NAME,
			                                         sizeof( patientRec.scheduledStationName ),
				                                     patientRec.scheduledStationName );

	                /* We do get MC_NULL_VALUE as return status from Merge SCP.
		            ** Merge said that means the server supports this attribute
			        ** and the return value is NULL.
				    */
	                if ( status == MC_NULL_VALUE )
		            {
			            if (errorReportTimesCount < 2)
				            Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `scheduledStationName'");
					}
	                else if (status != MC_NORMAL_COMPLETION )
		            {
			            if (errorReportTimesCount < 2)
				            Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `scheduled station name'.", __LINE__, status );
//					      MC_Free_Message( &responseMessageID );
//						  throw( WORKLISTUTILS_SOFTWARE );
	                }
				}

                /*
                ** When given a scheduled procedure step ID, we want to fill
                ** that in as well. (0040,0009)
                */
				if ( !_attributeBlocked[scheduledProcedureStepID] )
				{
	                status = MC_Get_Value_To_String( procedureStepItemNum,
		                                             MC_ATT_SCHEDULED_PROCEDURE_STEP_ID,
			                                         sizeof( patientRec.scheduledProcedureStepID ),
				                                     patientRec.scheduledProcedureStepID );
					if ( status == MC_NULL_VALUE )
	                {
		                if (errorReportTimesCount < 2)
			                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `scheduledProcedureStepID'");
					}
	                else if (status != MC_NORMAL_COMPLETION )
		            {
			            if (errorReportTimesCount < 2)
				            Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `scheduled procedure step id'.", __LINE__, status );
//					      MC_Free_Message( &responseMessageID );
//						  throw( WORKLISTUTILS_SOFTWARE );
					}
				}

                /*
                ** Since the Scheduled Protocol Code Sequence (0040,0008) is contained
                ** within a sequence, we will attempt to open that sequence as an item:
                */
                status = MC_Get_Value_To_Int( procedureStepItemNum,
                                              MC_ATT_SCHEDULED_ACTION_ITEM_CODE_SEQUENCE,
                                              &procedureCodeItemNum );
                if ( status == MC_NULL_VALUE )
                {
                    if (errorReportTimesCount < 2)
                        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for MC_ATT_SCHEDULED_ACTION_ITEM_CODE_SEQUENCE and `scheduledProtocolCodeSequence' could not be extracted.");
				}
                else if ( status != MC_NORMAL_COMPLETION )
                {
                    if (errorReportTimesCount < 2)
                        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `scheduled protocol code sequence id'.", __LINE__, status );
//                    MC_Free_Message( &responseMessageID );
//                    throw( WORKLISTUTILS_SOFTWARE );
                }
                else
                {
                    /*
                    ** If we have a scheduled protocol code value, then fill it in. (0008,0100)
                    */
					if ( !_attributeBlocked[scheduledProtocolCodeValue] )
					{
			            status = MC_Get_Value_To_String( procedureCodeItemNum,
				                                         MC_ATT_CODE_VALUE,
					                                     sizeof( patientRec.scheduledProtocolCodeValue ),
						                                 patientRec.scheduledProtocolCodeValue );
	                    if ( status == MC_NULL_VALUE )
		                {
			                if (errorReportTimesCount < 2)
				                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `scheduledProtocolCodeValue'");
						}
	                    else if (status != MC_NORMAL_COMPLETION )
		                {
			                if (errorReportTimesCount < 2)
				                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `scheduled protocol code value'.", __LINE__, status );
//					          MC_Free_Message( &responseMessageID );
//						      throw( WORKLISTUTILS_SOFTWARE );
						}
					}

                    /*
                    ** If we have a scheduled protocol coding scheme designator, then fill it in. (0008,0102)
                    */
					if ( !_attributeBlocked[scheduledProtocolCodingSchemeDesignator] )
					{
			            status = MC_Get_Value_To_String( procedureCodeItemNum,
				                                         MC_ATT_CODING_SCHEME_DESIGNATOR,
					                                     sizeof( patientRec.scheduledProtocolCodingSchemeDesignator ),
						                                 patientRec.scheduledProtocolCodingSchemeDesignator );
	                    if ( status == MC_NULL_VALUE )
		                {
			                if (errorReportTimesCount < 2)
				                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `scheduledProtocolCodingSchemeDesignator'");
						}
	                    else if (status != MC_NORMAL_COMPLETION )
		                {
			                if (errorReportTimesCount < 2)
				                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `scheduled protocol coding scheme designator'.", __LINE__, status );
//					          MC_Free_Message( &responseMessageID );
//						      throw( WORKLISTUTILS_SOFTWARE );
						}
					}

					/*
                    ** If we have a scheduled protocol code meaning, then fill it in. (0008,0104)
                    */
					if ( !_attributeBlocked[scheduledProtocolCodeMeaning] )
					{
			            status = MC_Get_Value_To_String( procedureCodeItemNum,
				                                         MC_ATT_CODE_MEANING,
					                                     sizeof( patientRec.scheduledProtocolCodeMeaning ),
						                                 patientRec.scheduledProtocolCodeMeaning );
	                    if ( status == MC_NULL_VALUE )
		                {
			                if (errorReportTimesCount < 2)
				                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `scheduledProtocolCodeMeaning'");
						}
	                    else if (status != MC_NORMAL_COMPLETION )
		                {
			                if (errorReportTimesCount < 2)
				                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `scheduled protocol code meaning'.", __LINE__, status );
//					          MC_Free_Message( &responseMessageID );
//						      throw( WORKLISTUTILS_SOFTWARE );
						}
					}
                }

#ifdef MPPS_ATTR
				/*
                ** Since the Referenced Study Component Sequence (0008,1111) is contained
                ** within a sequence, we will attempt to open that sequence as an item:
                */
                status = MC_Get_Value_To_Int( procedureStepItemNum,
                                              MC_ATT_REFERENCED_STUDY_COMPONENT_SEQUENCE,
                                              &referencedStudyComponentItemNum );
                if ( status == MC_NULL_VALUE )
                {
                    if (errorReportTimesCount < 2)
                        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for MC_ATT_REFERENCED_STUDY_COMPONENT_SEQUENCE and `Referenced Study Component Sequence' could not be extracted.");
				}
                else if ( status != MC_NORMAL_COMPLETION )
                {
                    if (errorReportTimesCount < 2)
                        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `referenced Study Component sequence id'.", __LINE__, status );
//                    MC_Free_Message( &responseMessageID );
//                    throw( WORKLISTUTILS_SOFTWARE );
                }
                else
                {
                    /*
                    ** If we have a referenced component SOP class UID, then fill it in. (0008,1150)
                    */
					if ( !_attributeBlocked[referencedComponentSOPClassUID] )
					{
			            status = MC_Get_Value_To_String( referencedStudyComponentItemNum,
				                                         MC_ATT_REFERENCED_SOP_CLASS_UID,
					                                     sizeof( patientRec.referencedComponentSOPClassUID ),
						                                 patientRec.referencedComponentSOPClassUID );
	                    if ( status == MC_NULL_VALUE )
		                {
			                if (errorReportTimesCount < 2)
				                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `referencedComponentSOPClassUID'");
						}
	                    else if (status != MC_NORMAL_COMPLETION )
		                {
			                if (errorReportTimesCount < 2)
				                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `referenced component SOP class UID'.", __LINE__, status );
//					          MC_Free_Message( &responseMessageID );
//						      throw( WORKLISTUTILS_SOFTWARE );
						}
					}

                    /*
                    ** If we have a referenced component SOP instance UID, then fill it in. (0008,1155)
                    */
					if ( !_attributeBlocked[referencedComponentSOPInstanceUID] )
					{
			            status = MC_Get_Value_To_String( referencedStudyComponentItemNum,
				                                         MC_ATT_REFERENCED_SOP_INSTANCE_UID,
					                                     sizeof( patientRec.referencedComponentSOPInstanceUID ),
						                                 patientRec.referencedComponentSOPInstanceUID );
	                    if ( status == MC_NULL_VALUE )
		                {
			                if (errorReportTimesCount < 2)
				                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `referencedComponentSOPInstanceUID'");
						}
	                    else if (status != MC_NORMAL_COMPLETION )
		                {
			                if (errorReportTimesCount < 2)
				                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `referenced component SOP instance UID'.", __LINE__, status );
//					          MC_Free_Message( &responseMessageID );
//						      throw( WORKLISTUTILS_SOFTWARE );
						}
					}
                }
#endif
            }

            /*
            ** If given a requested procedure id, then we read it in. (0040,1001)
            */
			if ( !_attributeBlocked[requestedProcedureID] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_REQUESTED_PROCEDURE_ID,
			                                     sizeof( patientRec.requestedProcedureID ),
				                                 patientRec.requestedProcedureID );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `requestedProcedureID'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `requested procedure id'.", __LINE__, status );
//				      MC_Free_Message( &responseMessageID );
//					  throw( WORKLISTUTILS_SOFTWARE );
			    }
			}

            /*
            ** If given a requested procedure description, then we read it in. (0032,1060)
            */
			if ( !_attributeBlocked[requestedProcedureDescription] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_REQUESTED_PROCEDURE_DESCRIPTION,
			                                     sizeof( patientRec.requestedProcedureDescription ),
				                                 patientRec.requestedProcedureDescription );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `requestedProcedureDescription'");
				}
	            else if (status != MC_NORMAL_COMPLETION )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `requested procedure description'.", __LINE__, status );
//		              MC_Free_Message( &responseMessageID );
//			          throw( WORKLISTUTILS_SOFTWARE );
				}
			}

            /*
            ** Since some of the responses are contained within a sequence,
            ** we will attempt to open that sequence as an item:
            */
            status = MC_Get_Value_To_Int( responseMessageID,
                                          MC_ATT_REQUESTED_PROCEDURE_CODE_SEQUENCE,
                                          &procedureCodeItemNum );
            if ( status == MC_NULL_VALUE )
            {
                if (errorReportTimesCount < 2)
                    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for REQUESTED_PROCEDURE_CODE_SEQUENCE and `requestedProcedureCodeMeaning' could not be extracted.");
			}
            else if ( status != MC_NORMAL_COMPLETION )
            {
                if (errorReportTimesCount < 2)
                    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `requested procedure code sequence id'.", __LINE__, status );
//                MC_Free_Message( &responseMessageID );
//                throw( WORKLISTUTILS_SOFTWARE );
            }
            else
            {
                /*
                ** If we have a code meaning for Requested Procedure, then fill it in. (0008,0104)
                */
				if ( !_attributeBlocked[requestedProcedureCodeMeaning] )
				{
	                status = MC_Get_Value_To_String( procedureCodeItemNum,
		                                             MC_ATT_CODE_MEANING,
			                                         sizeof( patientRec.requestedProcedureCodeMeaning ),
				                                     patientRec.requestedProcedureCodeMeaning );
				    if ( status == MC_NULL_VALUE )
					{
						if (errorReportTimesCount < 2)
							Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `requestedProcedureCodeMeaning'");
					}
					else if (status != MC_NORMAL_COMPLETION )
					{
						if (errorReportTimesCount < 2)
							Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `requested procedure code meaning'.", __LINE__, status );
//						MC_Free_Message( &responseMessageID );
//						throw( WORKLISTUTILS_SOFTWARE );
					}
				}
            }

            /*
            ** If we have a study instance UID, then fill it in. (0020,000D)
            */
			if ( !_attributeBlocked[studyInstanceUID] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_STUDY_INSTANCE_UID,
			                                     sizeof( patientRec.studyInstanceUID ),
				                                 patientRec.studyInstanceUID );
				if ( status == MC_NULL_VALUE )
				{
					if (errorReportTimesCount < 2)
						Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `studyInstanceUID'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `study instance UID'.", __LINE__, status );
//					  MC_Free_Message( &responseMessageID );
//					  throw( WORKLISTUTILS_SOFTWARE );
			    }
			}

            /*
            ** If we have an accession number, then fill it in. (0008,0050)
            */
			if ( !_attributeBlocked[accessionNumber] )
			{
				status = MC_Get_Value_To_String( responseMessageID,
					                             MC_ATT_ACCESSION_NUMBER,
						                         sizeof( patientRec.accessionNumber ),
							                     patientRec.accessionNumber );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `accessionNumber'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
		         if (errorReportTimesCount < 2)
			            Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `accession number'.", __LINE__, status );
//				      MC_Free_Message( &responseMessageID );
//					  throw( WORKLISTUTILS_SOFTWARE );
			    }
			}

            /*
            ** If we have a requesting physician, then fill it in. (0032,1032)
            */
			if ( !_attributeBlocked[requestingPhysician] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_REQUESTING_PHYSICIAN,
			                                     sizeof( patientRec.requestingPhysician ),
				                                 patientRec.requestingPhysician );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `requestingPhysician'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `requesting physician'.", __LINE__, status );
//				      MC_Free_Message( &responseMessageID );
//					  throw( WORKLISTUTILS_SOFTWARE );
			    }
			}

            /*
            ** If we have a referring physician's name, then fill it in. (0008,0090)
            */
			if ( !_attributeBlocked[referringPhysician] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_REFERRING_PHYSICIANS_NAME,
			                                     sizeof( patientRec.referringPhysician ),
				                                 patientRec.referringPhysician );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `referringPhysician'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `referring physician's name'.", __LINE__, status );
//				      MC_Free_Message( &responseMessageID );
//				      throw( WORKLISTUTILS_SOFTWARE );
			    }
			}

            /*
            ** Now we grab the patient's name (0010,0010)
            */
			if ( !_attributeBlocked[patientsName] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_PATIENTS_NAME,
			                                     sizeof( patientRec.patientsName ),
				                                 patientRec.patientsName );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `patientsName'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `patient's name'.", __LINE__, status );
//				      MC_Free_Message( &responseMessageID );
//					  throw( WORKLISTUTILS_SOFTWARE );
				}
			}

            /*
            ** Now we grab the patient's ID (0010,0020)
            */
			if ( !_attributeBlocked[patientID] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_PATIENT_ID,
			                                     sizeof( patientRec.patientID ),
				                                 patientRec.patientID );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `patientID'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `patient's id'.", __LINE__, status );
//					  MC_Free_Message( &responseMessageID );
//					  throw( WORKLISTUTILS_SOFTWARE );
				}
			}

            /*
            ** Patient's date of birth. (0010,0030)
            */
			if ( !_attributeBlocked[patientsDOB] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_PATIENTS_BIRTH_DATE,
			                                     sizeof( patientRec.patientsDOB ),
				                                 patientRec.patientsDOB );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `patientsDOB'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `patient's date of birth'.", __LINE__, status );
//		              MC_Free_Message( &responseMessageID );
//			          throw( WORKLISTUTILS_SOFTWARE );
				}
			}

            /*
            ** Patient's height. (0010,1020)
            */
			if ( !_attributeBlocked[patientsHeight] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_PATIENTS_SIZE,
			                                     sizeof( patientRec.patientsHeight ),
				                                 patientRec.patientsHeight );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `patientsHeight'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `patient's height'.", __LINE__, status );
// We commented out the following 2 lines because we got MC_INVALID_TAG from Merge
//			          MC_Free_Message( &responseMessageID );
//				      throw( WORKLISTUTILS_SOFTWARE );
			    }
			}

            /*
            ** Patient's weight. (0010,1030)
            */
			if ( !_attributeBlocked[patientsWeight] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_PATIENTS_WEIGHT,
			                                     sizeof( patientRec.patientsWeight ),
				                                 patientRec.patientsWeight );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `patientsWeight'");
				}
				else if (status != MC_NORMAL_COMPLETION )
				{
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `patient's weight'.", __LINE__, status );
//				      MC_Free_Message( &responseMessageID );
//					  throw( WORKLISTUTILS_SOFTWARE );
			    }
			}

            /*
            ** Patient's gender. (0010,0040)
            */
			if ( !_attributeBlocked[patientsGender] )
			{
				status = MC_Get_Value_To_String( responseMessageID,
					                             MC_ATT_PATIENTS_SEX,
						                         sizeof( patientRec.patientsGender ),
							                     patientRec.patientsGender );
				if ( status == MC_NULL_VALUE )
	            {
		            if (errorReportTimesCount < 2)
			            Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `patientsGender'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `patient's gender'.", __LINE__, status );
//				      MC_Free_Message( &responseMessageID );
//					  throw( WORKLISTUTILS_SOFTWARE );
			    }
			}

            /*
            ** Patient state. (0038,0500)
            */
			if ( !_attributeBlocked[patientState] )
			{
				status = MC_Get_Value_To_String( responseMessageID,
					                             MC_ATT_PATIENT_STATE,
						                         sizeof( patientRec.patientState ),
							                     patientRec.patientState );
			    if ( status == MC_NULL_VALUE )
				{
	                if (errorReportTimesCount < 2)
		                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `patientState'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `patient state'.", __LINE__, status );
//				      MC_Free_Message( &responseMessageID );
//					  throw( WORKLISTUTILS_SOFTWARE );
				}
			}

            /*
            ** Pregnancy status. (0010,21C0)
            */
			if ( !_attributeBlocked[pregnancyStatus] )
			{
	            tempPregnancyStatus = 0;
		        status = MC_Get_Value_To_UInt( responseMessageID,
			                                   MC_ATT_PREGNANCY_STATUS,
				                               &tempPregnancyStatus);
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `pregnancyStatus'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `pregnancy status'.", __LINE__, status );
//			          MC_Free_Message( &responseMessageID );
//				      throw( WORKLISTUTILS_SOFTWARE );
			    }

				if (tempPregnancyStatus>4 || tempPregnancyStatus<0)
				{
					tempPregnancyStatus = 0;
				    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. MWL returns PregnancyStatus index out of range 0-4.", __LINE__, status );
				}
				sprintf( patientRec.pregnancyStatus, "%s", strPregStat[tempPregnancyStatus] );
			}

            /*
            ** Medical alerts. (0010,2000)
            */
			if ( !_attributeBlocked[medicalAlerts] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_MEDICAL_ALERTS,
			                                     sizeof( patientRec.medicalAlerts ),
				                                 patientRec.medicalAlerts );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `medicalAlerts'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `medical alerts'.", __LINE__, status );
//			          MC_Free_Message( &responseMessageID );
//				      throw( WORKLISTUTILS_SOFTWARE );
			    }
			}

            /*
            ** Contrast allergies. (0010,2110)
            */
			if ( !_attributeBlocked[contrastAllergies] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_CONTRAST_ALLERGIES,
			                                     sizeof( patientRec.contrastAllergies ),
				                                 patientRec.contrastAllergies );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `contrastAllergies'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `contrast allergies'.", __LINE__, status );
//			          MC_Free_Message( &responseMessageID );
//				      throw( WORKLISTUTILS_SOFTWARE );
			    }
			}

            /*
            ** Special needs. (0038,0050)
            */
			if ( !_attributeBlocked[specialNeeds] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_SPECIAL_NEEDS,
			                                     sizeof( patientRec.specialNeeds ),
				                                 patientRec.specialNeeds );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `specialNeeds'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `special needs'.", __LINE__, status );
//			          MC_Free_Message( &responseMessageID );
//				      throw( WORKLISTUTILS_SOFTWARE );
				}
			}

            /*
            ** Other Patient's ID. (0010,1000)
            */
			if ( !_attributeBlocked[otherPatientID] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_OTHER_PATIENT_IDS,
			                                     sizeof( patientRec.otherPatientID ),
				                                 patientRec.otherPatientID );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `otherPatientID'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `other patient's ID'.", __LINE__, status );
// We commented out the following 2 lines because we got MC_INVALID_TAG from Merge
//					MC_Free_Message( &responseMessageID );
//					throw( WORKLISTUTILS_SOFTWARE );
				}
			}

            /*
            ** Intended Recipients. (0040,1010)
            */
			if ( !_attributeBlocked[intendedRecipients] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_NAMES_OF_INTENDED_RECIPIENTS_OF_RESULTS,
			                                     sizeof( patientRec.intendedRecipients ),
				                                 patientRec.intendedRecipients );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `intendedRecipients'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `intended recipients'.", __LINE__, status );
// We commented out the following 2 lines because we got MC_INVALID_TAG from Merge
//			          MC_Free_Message( &responseMessageID );
//				      throw( WORKLISTUTILS_SOFTWARE );
			    }
			}

            /*
            ** Image Service Request Comments. (0040,2400)
            */
			if ( !_attributeBlocked[imageServiceRequestComments] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_IMAGING_SERVICE_REQUEST_COMMENTS,
			                                     sizeof( patientRec.imageServiceRequestComments ),
				                                 patientRec.imageServiceRequestComments );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `imageServiceRequestComments'");
				}
				else if (status != MC_NORMAL_COMPLETION )
	            {
		            if (errorReportTimesCount < 2)
			            Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `image service request comments'.", __LINE__, status );
// We commented out the following 2 lines because we got MC_INVALID_TAG from Merge
//				      MC_Free_Message( &responseMessageID );
//					  throw( WORKLISTUTILS_SOFTWARE );
			    }
			}

            /*
            ** Requested Procedure Comments. (0040,1400)
            */
			if ( !_attributeBlocked[requestedProcedureComments] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_REQUESTED_PROCEDURE_COMMENTS,
			                                     sizeof( patientRec.requestedProcedureComments ),
				                                 patientRec.requestedProcedureComments );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `requestedProcedureComments'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
	                if (errorReportTimesCount < 2)
		                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `requested procedure comments'.", __LINE__, status );
// We commented out the following 2 lines because we got MC_INVALID_TAG from Merge
//			          MC_Free_Message( &responseMessageID );
//				      throw( WORKLISTUTILS_SOFTWARE );
			    }
			}

#ifdef INVALID_TAG
// Merge library thinks this is an invalid tag for MWL
            /*
            ** Series Instance UID. (0020,000E)
            */
			if ( !_attributeBlocked[seriesInstanceUID] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_SERIES_INSTANCE_UID,
			                                     sizeof( patientRec.seriesInstanceUID ),
				                                 patientRec.seriesInstanceUID );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `seriesInstanceUID'");
				}
		        else if (status != MC_NORMAL_COMPLETION )
			    {
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `series instance UID'.", __LINE__, status );
//					MC_Free_Message( &responseMessageID );
//					throw( WORKLISTUTILS_SOFTWARE );
				}
			}
#endif
            /*
            ** Reason For Requested Procedure. (0040,1002)
            */
			if ( !_attributeBlocked[reasonForRequestedProcedure] )
			{
	            status = MC_Get_Value_To_String( responseMessageID,
		                                         MC_ATT_REASON_FOR_THE_REQUESTED_PROCEDURE,
			                                     sizeof( patientRec.reasonForRequestedProcedure ),
				                                 patientRec.reasonForRequestedProcedure );
	            if ( status == MC_NULL_VALUE )
		        {
			        if (errorReportTimesCount < 2)
				        Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `reasonForRequestedProcedure'");
				}
				else if (status != MC_NORMAL_COMPLETION )
			    {
				    if (errorReportTimesCount < 2)
					    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `reason for requested procedure'.", __LINE__, status );
// We commented out the following 2 lines because we got MC_INVALID_TAG from Merge
//					MC_Free_Message( &responseMessageID );
//					throw( WORKLISTUTILS_SOFTWARE );
				}
			}

            /*
            ** Since the Referenced Study Sequence (0008,1110) is a sequence,
            ** we will attempt to open that sequence as an item:
            */
            status = MC_Get_Value_To_Int( responseMessageID,
                                          MC_ATT_REFERENCED_STUDY_SEQUENCE,
                                          &referencedStudyItemNum );
            if ( status == MC_NULL_VALUE )
            {
                if (errorReportTimesCount < 2)
                    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for MC_ATT_REFERENCED_STUDY_SEQUENCE and `Referenced Study Sequence' could not be extracted.");
			}
            else if ( status != MC_NORMAL_COMPLETION )
            {
                if (errorReportTimesCount < 2)
                    Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `referenced Study sequence id'.", __LINE__, status );
//                MC_Free_Message( &responseMessageID );
//                throw( WORKLISTUTILS_SOFTWARE );
            }
            else
            {
                /*
                ** If we have a referenced SOP class UID, then fill it in. (0008,1150)
                */
				if ( !_attributeBlocked[referencedSOPClassUID] )
				{
			        status = MC_Get_Value_To_String( referencedStudyItemNum,
				                                     MC_ATT_REFERENCED_SOP_CLASS_UID,
					                                 sizeof( patientRec.referencedSOPClassUID ),
						                             patientRec.referencedSOPClassUID );
	                if ( status == MC_NULL_VALUE )
		            {
			            if (errorReportTimesCount < 2)
				            Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `referencedSOPClassUID'");
					}
	                else if (status != MC_NORMAL_COMPLETION )
		            {
			            if (errorReportTimesCount < 2)
				            Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `referenced SOP class UID'.", __LINE__, status );
//					      MC_Free_Message( &responseMessageID );
//						  throw( WORKLISTUTILS_SOFTWARE );
					}
				}

                /*
                ** If we have a referenced SOP instance UID, then fill it in. (0008,1155)
                */
				if ( !_attributeBlocked[referencedSOPInstanceUID] )
				{
			        status = MC_Get_Value_To_String( referencedStudyItemNum,
				                                     MC_ATT_REFERENCED_SOP_INSTANCE_UID,
					                                 sizeof( patientRec.referencedSOPInstanceUID ),
						                             patientRec.referencedSOPInstanceUID );
	                if ( status == MC_NULL_VALUE )
		            {
			            if (errorReportTimesCount < 2)
				            Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg got NULL_VALUE for `referencedSOPInstanceUID'");
					}
	                else if (status != MC_NORMAL_COMPLETION )
		            {
			            if (errorReportTimesCount < 2)
				            Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to read the `referenced SOP instance UID'.", __LINE__, status );
//					      MC_Free_Message( &responseMessageID );
//						  throw( WORKLISTUTILS_SOFTWARE );
					}
				}
            }

            /*
            ** Attempt to free the message, and then wait for the next one
            ** from the server.
            */
            status = MC_Free_Message( &responseMessageID );
            if ( status != MC_NORMAL_COMPLETION )
            {
                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to free message:   %d", __LINE__, status, responseMessageID );
                LLInsert( p_list, &patientRec );
                throw( WORKLISTUTILS_SOFTWARE );
            }

            /*
            ** The above "..Value_To_String" calls grab the actual responses
            ** from the server.  We took these responses and stuffed them into
            ** a structure. At this point, We push the information onto a list
            ** for future usage...
            */
            if ( LLInsert( p_list, &patientRec ) != MERGE_SUCCESS )
            {
                throw( WORKLISTUTILS_SOFTWARE );
            }


            /*
            ** We've read a message.  If we've read more messages than
            ** what we are looking for, then we want to send a
            ** C_CANCEL_RQ back to the SCP.
            */
            messageCount++;
            if ( ( messageCount >= _threshold ) &&
                 ( alreadySent != true ) )
            {
                /*
                ** We use these two variables to make sure that we
                ** only send ONE cancel message to the SCP.
                */
                messageCount = 0;
                alreadySent = true;

                try
                {
                      sendCcancelRQ( p_associationID );
                }
                catch (...)
                {
                     throw( WORKLISTUTILS_SOFTWARE );
                }
            }
    
        }
        else if( response == C_FIND_CANCEL_REQUEST_RECEIVED )
        {
          /*
          ** In this case, the SCP is acknowledging our sending of a
          ** request to cancel the query.
          */
          Message( MWARNING, MLoverall | toDeveloper, "Received acknowledgement of a C_CANCEL_RQ message..." );

          /*
          ** Attempt to free the message, and then wait for the next one
          ** from the server.
          */
          status = MC_Free_Message( &responseMessageID );
          if ( status != MC_NORMAL_COMPLETION )
          {
              Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error attempting to free message:   %d", __LINE__, status, responseMessageID );
              throw( WORKLISTUTILS_MEMORY );
          }

          /*
          ** There are actually two choices.  This code will keep processing
          ** response messages after this SCU has sent out the CANCEL request.
          ** Other designs would need to handle responses, but might not
          ** process the responses until this acknowledgement is received.
          ** In this case, we want to stop looking for responses once we've
          ** received the acknowledgement of the C_CANCEL from the SCP --
          ** hence the "break".
          */
          break;
        }
        else
        {
            /*
            ** OK, now we've gotten an unexpected response from the server.
            ** We can panic now, but we'll attempt to tell the user why...
            */
            Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Obtained an unknown response message from the SCP. Response:  %X.=n", __LINE__, MC_NORMAL_COMPLETION, response );
            MC_List_Message( responseMessageID, NULL );
            status = MC_Free_Message( &responseMessageID );
            if ( status != MC_NORMAL_COMPLETION )
            {
                Message( MWARNING, MLoverall | toDeveloper, "processWorklistReplyMsg: line %d, status %d. Error freeing message:  %d", __LINE__, status, responseMessageID );
                throw( WORKLISTUTILS_MEMORY );
            }
            break; /* nothing should come after this... */

        } /* End of giant if-then-else */

    } /* End of infinite loop */

#ifdef REQUIRES_MERGE_LEAKFIX 
	PegasusLibThreadTerm();
#endif

} /* End of processWorklistReplyMsg */


/*****************************************************************************
**
** NAME
**    sendCcancelRQ
**
** SYNOPSIS
**    void sendCcancelRQ
**
** ARGUMENTS
**    int  p_associationID - the association ID
**
** DESCRIPTION
**    This function will send a C_CANCEL_RQ message to the SCP.  It is
**    used to tell the SCP that we've received enough, or too much, data
**    and that it should stop sending data to us.
**
** RETURNS
**    None
**
** SEE ALSO
**    
*****************************************************************************/
void WorklistUtils::sendCcancelRQ(int  p_associationID )
                                  throw (WorklistUtils::WorklistUtilsException)
{
    int       messageID;
    MC_STATUS status;

    status = MC_Open_Message( &messageID, "MODALITY_WORKLIST_FIND",
                              C_CANCEL_RQ );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendCcancelRQ: line %d, status %d. Unable to open a message for MODALITY_WORKLIST_FIND", __LINE__, status );
        throw ( WORKLISTUTILS_SOFTWARE );
    }
    
    Message( MWARNING, MLoverall | toDeveloper, "Sending a C_CANCEL_RQ..." );

    status = MC_Send_Request_Message( p_associationID, messageID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        MC_Free_Message ( &messageID );
        Message( MWARNING, MLoverall | toDeveloper, "sendCcancelRQ: line %d, status %d. Unable to send the cancel message to the SCP.", __LINE__, status );
        throw ( WORKLISTUTILS_SOFTWARE );
    }
    
    status = MC_Free_Message( &messageID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        Message( MWARNING, MLoverall | toDeveloper, "sendCcancelRQ: line %d, status %d. Error attempting to free message:   %d", __LINE__, status, messageID );
        throw ( WORKLISTUTILS_MEMORY );
    }
}


/*****************************************************************************
**
** NAME
**    getAttributeFilter
**
** SYNOPSIS
**    void getAttributeFilter
**
** ARGUMENTS
**
** DESCRIPTION
**    This function reads from worklistattributefilter.txt
**    and create the filter for the attributes that the user
**    wants the worklist to return.
**
** RETURNS
**    None
**
*****************************************************************************/
void WorklistUtils::getAttributeFilter()
{
  char line[256], *p;
  unsigned int i;

  memset(_attributeBlocked, 0, sizeof(_attributeBlocked));

  std::ifstream attributeFile("data/Facility/Worklist/worklistattributefilter.txt");
  if (!attributeFile)
    {
      Message( MWARNING, MLoverall | toDeveloper, "Unable to open \"data/Facility/Worklist/worklistattributefilter.txt\" for reading.");
	  return;
    }

  memset(line, 0, sizeof(line));
  while(!attributeFile.getline(line, sizeof(line)).eof())
  { 
	if (isalpha(line[0]) && line[0] != 'T' && line[0] != 't')
	  {
		  p = line;
		  while (!isdigit(*p)) p++;
		  i = atoi(p);
		  if(i>0 && i<=sizeof(_attributeBlocked))
			_attributeBlocked[i-1]=1;
	  }

      memset(line, 0, sizeof(line));
  }
}
