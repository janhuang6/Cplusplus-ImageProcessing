
/*************************************************************************
 *
 *       System: MergeCOM-3 Integrator's Tool Kit
 *
 *    $Workfile: work_scu.c $
 *
 *    $Revision: 1.1 $
 *
 *  Description: 
 *               Modality Worklist Service Class User Application.
 *
 ************************************************************************/

/*
 *  Run-time version number string
 */
static char id_string[] = "$Header: /usr/adac/Repository/camera/camera/control/src/worklist/work_scus/work_scu.c,v 1.1 Exp $";

/* $NoKeywords: $ */


/* Default is unix */
#if !defined(_MSDOS)         && !defined(_RMX3)   && !defined(INTEL_WCC) && \
    !defined(_ALPHA_OPENVMS) && !defined(OS9_68K) && !defined(UNIX) 
#define UNIX            1
#endif

/* Defined for MACINTOSH */
#if defined(_MACINTOSH)
#include <Types.h>
#include <Events.h>
#include <Menus.h>
#include <Windows.h>
#include <time.h>
#include <console.h>
#include <SIOUX.h>
#endif

/*
** We define INPUT_LEN to be 256.  This is the number of characters that the
** user can type before we overflow.
*/
#define INPUT_LEN 256

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>

#include "mergecom.h" 
#include "mc3msg.h"
#include "mcstatus.h"
#include "diction.h"
#include "qr.h"

typedef struct _WORK_PATIENT_REC
{
    char    startDate[ DA_LEN ];
    char    startTime[ TM_LEN ];
    char    modality[ CS_LEN ];
    char    stepID[ SH_LEN ];        /* Scheduled Procedure Step ID */
    char    stepDesc[ LO_LEN ];      /* Scheduled Procedure Step Description */
    char    physicianName[ PN_LEN ];
    char    procedureID[ SH_LEN ];   /* Requested Procedure ID */
    char    procedureDesc[ LO_LEN ]; /* Requested Procedure Description */
    char    studyInstance[ UI_LEN ];
    char    accession[ SH_LEN ];
    char    patientName[ PN_LEN ];
    char    patientID[ LO_LEN ];
    char    patientBirthDate[ DA_LEN ];
    char    patientSex[ CS_LEN ];
} WORK_PATIENT_REC;


/* Global variables */
static char G_appTitle[ AE_LEN ]="MERGE_WORK_SCU";
static char G_destAppTitle[ AE_LEN ]="";
static int  G_portNumber=0;
static char G_remoteHost[ AE_LEN ]="";
static int  G_timeout=3000;
static int  G_threshold=15; /* About one screenfull */

/* Function prototypes */
static int              GetOptions( int               argc, 
                                    char            **argv );
static int   PerformInitialization( int              *A_applicationID );
static int         OpenAssociation( int               A_applicationID,
                                    int              *A_associationID );
static void       DisplayAndPrompt( char             *A_userInput,
                                    char             *A_value,
                                    char             *A_modality,
                                    char             *A_aeTitle,
                                    char             *A_patientName,
                                    char             *A_startDate );
static int   SetAndSendWorklistMsg( int               A_associationID,
                                    char             *A_modality,
                                    char             *A_aeTitle,
                                    char             *A_startDate,
                                    char             *A_patientName,
                                    char             *A_patientID,
                                    char             *A_scheduledProcStepID);
static int ProcessWorklistReplyMsg( int               A_associationID,
                                    LIST             *A_list );
static int           SendCcancelRQ( int               A_associationID );
static int          GatherMoreData( int               A_applicationID,
                                    LIST             *A_list,
                                    char             *A_modality,
                                    char             *A_ae_title,
                                    char             *A_startDate );
static void            PerformMPPS( WORK_PATIENT_REC *A_patient,
                                    int               A_applicationID );
static int    SetNullOrStringValue( int               A_messageID,
                                    unsigned long     A_tag,
                                    char*             A_value,
                                    char*             A_function );
static int          SetStringValue( int               A_messageID,
                                    unsigned long     A_tag,
                                    char*             A_value,
                                    char*             A_function );
static int           SendNCREATERQ( int               A_applicationID,
                                    char             *A_affectedSOPinstance,
                                    WORK_PATIENT_REC *A_patient );
static int            SetNCREATERQ( int               A_messageID,
                                    WORK_PATIENT_REC *A_patient );
static int              SendNSETRQ( int               A_applicationID,
                                    char             *A_affectedSOP );
static int               SetNSETRQ( int               A_messageID,
                                    char             *A_affectedSOP );
static int          HandleStatProc( int               A_applicationID );
static int      SendNSETRQComplete( int               A_applicationID,
                                    char             *A_requestedSOPinstance,
                                    WORK_PATIENT_REC *A_patient );


/*****************************************************************************
**
** NAME
**    main - Modality worklist service class user program.
**
** SYNOPSIS
**    work_scu [options] remote_application_name
**
**    This program sends a C_FIND message to the service class provider
**    and waits for the results to be sent back.  The results are sent
**    back in the form of a C_FIND reply.
**
*****************************************************************************/

#ifdef VXWORKS
int  workscu(int argc, char** argv);
int workscu(int argc, char** argv)
#else
int  main(int argc, char** argv);
int main(int argc, char** argv)
#endif
{
    MC_STATUS     status;                /* The status returned via toolkit  */
    int           applicationID;         /* ID of this app, after registered */
    int           associationID;         /* ID of the association of this    */
                                         /* app, and the server.             */
    char          userInput[INPUT_LEN] = "";  /* Value that the user enters  */
    char          value[INPUT_LEN]="";   /* Value that the user enters       */

    char          modality[SH_LEN + 1] = ""; 
    char          aeTitle[PN_LEN + 1] = ""; 
    char          startDate[PN_LEN + 1] = ""; 
    char          patientName[PN_LEN + 1] = ""; 
    char          startTime[ 7 ] = "";
    char          entered[INPUT_LEN]="";
    LIST          patientList;           /* The list of patients to process  */


    /*
    ** The Macintosh needs to use the SIOUX window for input and output.
    ** The following code sets up the SIOUX options, and asks the user
    ** for the command line arguments.
    */
#if defined(_MACINTOSH) && defined(__MWERKS__)
    SIOUXSettings.initializeTB = true;
    SIOUXSettings.standalone = true;
    SIOUXSettings.setupmenus = true;
    SIOUXSettings.autocloseonquit = false;
    SIOUXSettings.asktosaveonclose = true;
    SIOUXSettings.showstatusline = true;
    argc = ccommand(&argv);
#endif 

    /*
    ** Obtain and parse any command line arguments.
    */
    if ( GetOptions( argc, argv ) == FAILURE )
    {
        /*
        ** We have no open associations here.  We can just exit.
        */
        exit( FAILURE );
    }

    /*
    ** We must initialize the toolkit, register this application,
    ** and form an association with the Service Class Provider.
    */
    if ( PerformInitialization( &applicationID ) != SUCCESS )
    {
        /*
        ** It is assumed that if initialization fails, then there will
        ** be little need in attempting to do any release of application.
        */
        exit( FAILURE );
    }

    /*
    ** To benefit the user, we default the date of the query to be the
    ** current date.
    */
    GetCurrentDate( startDate, startTime );

    /*
    ** Display the main menu and prompt the user for input, until the user
    ** decides to exit.
    */
    while( userInput[0] != 'X' )
    {
        strcpy( userInput, "" );
        strcpy( value, "" );
        /*
        ** Display the menu to the user and ask them for input
        */
        DisplayAndPrompt( userInput, value, modality, aeTitle, patientName,
                          startDate );
        /*
        ** Based on their input, we will decide how to continue.
        */
        switch( userInput[0] )
        {
            case '1':
                /*
                ** Modality
                */
                if ( ( strstr( value, "*" ) != NULL ) ||
                     ( strstr( value, "?" ) != NULL ) )
                {
                    printf( "Wildcard matching is illegal for this field.\n" );
                    printf( "Defaulting to NULL\n" );
                    value[0]='\0';
                    printf( "Press <return> to continue.\n" );
                    fgets( entered, sizeof( entered ), stdin );
                }
                strcpy( modality, value );
                break;
            case '2':
                /*
                ** Modality AE title
                */
                if ( ( strstr( value, "*" ) != NULL ) ||
                     ( strstr( value, "?" ) != NULL ) )
                {
                    printf( "Wildcard matching is illegal for this field.\n" );
                    printf( "Defaulting to NULL\n" );
                    value[0]='\0';
                    printf( "Press <return> to continue.\n" );
                    fgets( entered, sizeof( entered ), stdin );
                }
                strcpy( aeTitle, value );
                break;
            case '3':
                /*
                ** Patient Name
                */
                strcpy( patientName, value );
                break;
            case '4':
                /*
                ** Should the user attempt to enter an invalid date, we
                ** don't let them do so.  NOTE:  we let the other date
                ** functions parse a date range, such that we just do a
                ** simple sanity check here...
                */
                if ( strlen( value ) < 8 )
                {
                    printf( "A date string must be in the form of:\n"
                            " YYYYMMDD, YYYYMMDD-YYYYMMDD, -YYYMMDD, or "
                            "YYYMMDD-\n" );
                    printf( "Defaulting the date to today's date...\n" );
                    strcpy( value, "TODAY" );
                    printf( "Press <return> to continue.\n" );
                    fgets( entered, sizeof( entered ), stdin );
                }

                /*
                ** Procedure start date
                */
                if ( strcmp( value, "TODAY" ) == 0 )
                {
                    GetCurrentDate( value, startTime );
                }
                strcpy( startDate, value );
                break;
            case '5':
                /*
                ** Change the reply message cut-off threshold
                */
                if ( strtoul( value, NULL, 10 ) < 5 )
                {
                    printf( "Cannot set to a value, less than 5.\n" );
                    G_threshold = 5;
                    printf( "Press <return> to continue.\n" );
                    fgets( entered, sizeof( entered ), stdin );
                }
                else
                {
                    G_threshold = strtoul( value, NULL, 10 );
                }
                break;
            case '6':
                /*
                ** Handle a "STAT" procedure.  Perform a MPPS without having
                ** received a worklist from a worklist SCP.
                */
                HandleStatProc( applicationID );
                printf( "Press <return> to continue.\n" );
                fgets( entered, sizeof( entered ), stdin );
                break;
            case 'Q':
            case 'q':
                /*
                ** We need to create an empty list that will hold our
                ** query results.
                */
                if ( LLCreate( &patientList,
                               sizeof( WORK_PATIENT_REC ) ) != SUCCESS )
                {
                    LogError( ERROR_LVL_1, "main", __LINE__,
                              MC_NORMAL_COMPLETION, 
                              "Unable to initialize the linked list...\n" );
                    /*
                    ** This is a rather pesky situation.  We cannot continue
                    ** if we are unable to allocate memory for the linked list.
                    ** We will, however attempt to release this application
                    ** from the tooklit. 
                    */
                    MC_Release_Application( &applicationID );
                    exit( FAILURE );
                }

                /*
                ** Attempt to open an association with the provider.
                */
                if ( OpenAssociation( applicationID, &associationID ) !=
                     SUCCESS )
                {
                    /*
                    ** Even though we were unable to open the association
                    ** with the server, we will still attempt to release
                    ** this application from the library before exiting.
                    */
                    LLDestroy( &patientList );
                    printf( "Press <return> to continue.\n" );
                    fgets( entered, sizeof( entered ), stdin );
                    break;
                }

                /*
                ** At this point, the user has filled in all of their
                ** message parameters, and then decided to query.
                ** Therefore, we need to construct and send their message
                ** off to the server.
                */
                if ( SetAndSendWorklistMsg( associationID, modality, aeTitle,
                                            startDate, patientName, "", "" ) 
                                            == FAILURE )
                {
                    /*
                    ** Before exiting, we will attempt to close the association
                    ** that was successfully formed with the server.
                    */
                    LLDestroy( &patientList );
                    MC_Abort_Association( &associationID );
                    MC_Release_Application( &applicationID );
                    exit( FAILURE );
                }

                /*
                ** We've now sent out a CFIND message to a service class
                ** provider.  We will now wait for a reply from this server,
                ** and then "process" the reply.
                */
                if ( ProcessWorklistReplyMsg(  associationID, 
                                              &patientList ) == FAILURE )
                {
                    LLDestroy( &patientList );
                    exit( FAILURE );
                }

                /*
                ** Now that we are done, we'll close the association that
                ** we've formed with the server.
                */
                status = MC_Close_Association( &associationID );
                if ( status != MC_NORMAL_COMPLETION )
                {
                    LogError( ERROR_LVL_1, "main", __LINE__, status,
                              "Unable to close the association with "
                              "MERGE_WORK_SCP.\n" );
                    LLDestroy( &patientList );
                    exit ( FAILURE );
                }

                /*
                ** If we got no patients back from the server, tell the
                ** user that.
                */
                if ( LLNodes( &patientList ) == 0 )
                {
                    printf( "No patients were found based on the entered " );
                    printf( "criteria.\n" );
                    printf( "Press <return> to continue.\n" );
                    fgets( entered, sizeof( entered ), stdin );
                    break;
                }

                /*
                ** Now, we present the user with a secondary user menu,
                ** that will allow them to query the server again, and gain
                ** "deeper" information on the patients that they've
                ** selected for their modality.
                */
                if ( GatherMoreData( applicationID,
                                     &patientList, modality,
                                     aeTitle, startDate ) == FAILURE )
                {
                    exit( FAILURE );
                }

                /*
                ** Now destroy the patient list, because we will reuse
                ** it later...
                */
                LLDestroy( &patientList );

                break;
            case 'X':
            case 'x':
                /*
                ** The user chose to exit the program.
                */
                userInput[0]='X';
                break;
            default: /* Invalid response */
                printf( "'%s' is invalid.\n", userInput );
                printf( "Press <return> to continue.\n" );
                fgets( entered, sizeof( entered ), stdin );
        } /* End of switch */

    } /* End of while */

    /*
    ** The last thing that we do is to release this application from the
    ** library.
    */
    MC_Release_Application( &applicationID );

    if (MC_Library_Release() != MC_NORMAL_COMPLETION)
        printf("Error releasing the library.\n");

    exit( SUCCESS );

} /* End of main */



/*****************************************************************************
**
** NAME
**    PerformInitialization
**
** SYNOPSIS
**    Does the library initialization, registers this as an application,
**    opens an association with the server, and sets up some initial
**    program values
**
*****************************************************************************/
static int
PerformInitialization(
                        int *A_applicationID
                      )

{
    MC_STATUS    status;

    /*
    ** This is the initial call to the library.  It must be the first call
    ** to the toolkit that is made.
    */
    status = MC_Library_Initialization( NULL, NULL, NULL );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "PerformInitialization", __LINE__, status,
                  "Unable to initialize the library.\n" );
        return( FAILURE );
    }

    /*
    ** Once we've successfully initialized the library, then we must
    ** register this application with it.  "G_appTitle" is either
    ** defaulted to MERGE_WORK_SCU, or entered on the command line.
    */
    status = MC_Register_Application( A_applicationID, G_appTitle );

    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "PerformInitialization", __LINE__, status,
                  "Unable to register this application with the library.\n" );
        return( FAILURE );
    }

    return( SUCCESS );
}


/*****************************************************************************
**
** NAME
**    OpenAssociation
**
** SYNOPSIS
**    static int OpenAssociation
**
** ARGUMENTS
**    int  A_applicationID - The application ID 
**    int *A_associationID - returns the opened association 
**
** DESCRIPTION
**    Opens an association with the server based on the includance of,
**    or lack of, command line parameters.
**
** RETURNS
**    SUCCESS or FAILURE
**
** SEE ALSO
**
*****************************************************************************/
static int
OpenAssociation(
                   int  A_applicationID, 
                   int *A_associationID
               )
{
    int          *attemptPort = (int *)NULL;
    char         *attemptHost = (char *)NULL;
    MC_STATUS    status;

    if ( G_portNumber != 0 )
    {
        /*
        ** If we've been given a port number from the command line, then
        ** we want to make the attemptPort pointer point to this.  Hence,
        ** we make it point to the address of the G_portNumber.
        */
        attemptPort = &G_portNumber;
    }
    if ( G_remoteHost[0] != '\0' )
    {
        /*
        ** If we've been given a command line argument, then we want to
        ** make the attemptHost pointer point to a character string
        ** that contains the host we've passed.  Hence, the malloc and
        ** strcpy.
        */
        attemptHost = G_remoteHost;
    }

    /*
    ** Now that we've gotten the command line arguments either copied or left
    ** NULL, we can attempt to open the association...
    */
    status = MC_Open_Association( A_applicationID, A_associationID,
                                  G_destAppTitle, attemptPort,
                                  attemptHost, NULL );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "PerformInitialization", __LINE__, status,
                  "Unable to create an association with %s\n",
                  G_destAppTitle );
        return( FAILURE );
    }


    return( SUCCESS );
}


/*****************************************************************************
**
** NAME
**    DisplayAndPrompt
**
** SYNOPSIS
**    static void DisplayAndPrompt
**
** ARGUMENTS
**    char *A_userInput - used to pass what the user entered
**    char *A_value     - a value
**    char *A_modality  - the modality
**    char *A_aeTitle   - the AE title
**    char *A_startDate - the start date
**
** DESCRIPTION
**    Displays the main menu to the user and then prompts them for
**    input.
**
** RETURNS
**    nothing
**
** SEE ALSO
**
*****************************************************************************/
static void
DisplayAndPrompt(
                  char *A_userInput,
                  char *A_value,
                  char *A_modality,
                  char *A_aeTitle,
                  char *A_patientName,
                  char *A_startDate
                )
{
    char    entered[INPUT_LEN]="";
    int     i;

    printf( "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n" );
    printf( " ** MODALITY WORKLIST SEARCH **\n\n" );
    printf( "Enter the number of the value to modify,\n" );
    printf( "followed by the value that you wish to set.\n" );
    printf( " e.g.:  1 CT\n" );
    printf( "[1] Modality                   [%s]\n", A_modality );
    printf( "[2] Station Application Title  [%s]\n", A_aeTitle );
    printf( "[3] Patient Name               [%s]\n", A_patientName );
    printf( "[4] Procedure Start Date       [%s]\n", A_startDate );
    printf( "[5] Reply message cut-off threshold  [%d]\n", G_threshold );
    printf( "[6] STAT Procedure (MPPS without WORKLIST)\n" );
    printf( "[Q] Perform a query\n" );
    printf( "[X] Exit\n\n" );
    printf( "==>" );
    fflush( stdout );

    fgets( entered, sizeof( entered ), stdin );

    /*
    ** Strip off trailing non-ascii characters, ie c/r if it is there
    */
    for(i=(int)strlen(entered);i>=0;i--)
    {
#ifdef VXWORKS
        if ( (iscntrl)(entered[i]))
#else
        if (iscntrl(entered[i]))
#endif
            entered[i] = '\0'; 
        else
            break;
    }

    /*
    ** If the user has entered something that is at least 3 characters, then
    ** we'll attempt to parse everthing after the second character as an
    ** input value.
    */
    if( entered[2] != '\0' )
    {
        strcpy( A_value, &entered[2] );
    }

    strcpy( A_userInput, entered );
}



/*****************************************************************************
**
** NAME
**    SetNullOrStringValue
**
** SYNOPSIS
**    static int SetNullOrStringValue
**
** ARGUMENTS
**    int               A_messageID - the id of a message
**    unsigned long     A_tag       - the tag to set
**    char*             A_value     - the value to set
**    char*             A_function  - a function name to reference
**
** DESCRIPTION
**    This function is used to set a Type 2 attribute within a message.
**    It takes a string as input.  If the string is zero length, it
**    sets the tag to NULL, if not, it sets the tag to the actual value.
**
** RETURNS
**    SUCCESS or FAILURE
**
** SEE ALSO
**
*****************************************************************************/
static int
SetNullOrStringValue( 
                int               A_messageID,
                unsigned long     A_tag,
                char*             A_value,
                char*             A_function
            )
{
    MC_STATUS status;
    char      tagDescription[60];

    if ( strlen( A_value ) > 0 )
    {
        status = MC_Set_Value_From_String( A_messageID,
                                           A_tag,
                                           A_value );
        if ( status != MC_NORMAL_COMPLETION )
        {
            MC_Get_Tag_Info(A_tag, tagDescription, sizeof(tagDescription));
            LogError( ERROR_LVL_1, A_function, __LINE__, status,
                      "Failed to set the tag %s to \"%s\"\n",
                      tagDescription, A_value );
            return ( FAILURE );
        }
    }
    else
    {
        status = MC_Set_Value_To_NULL( A_messageID,
                                       A_tag );
        if ( status != MC_NORMAL_COMPLETION )
        {
            MC_Get_Tag_Info(A_tag, tagDescription, sizeof(tagDescription));
            LogError( ERROR_LVL_1, A_function, __LINE__, status,
                      "Failed to NULL \"%s\"\n", tagDescription );
            return ( FAILURE );
        }
    }
    
    return ( SUCCESS );
}



/*****************************************************************************
**
** NAME
**    SetStringValue
**
** SYNOPSIS
**    static int SetStringValue
**
** ARGUMENTS
**    int               A_messageID - the tag or message ID
**    unsigned long     A_tag       - the tag
**    char*             A_value     - the value to set
**    char*             A_function  - the calling function's name
**
** DESCRIPTION
**    This function is used to set string attributes within a message.
**    It takes a string as input and sets the value.  If a failure occurs,
**    it logs a warning message with the tag name.
**
** RETURNS
**    SUCCESS or FAILURE
**
** SEE ALSO
**    
*****************************************************************************/
static int
SetStringValue( 
                int               A_messageID,
                unsigned long     A_tag,
                char*             A_value,
                char*             A_function
            )
{
    MC_STATUS status;
    char      tagDescription[60];

    status = MC_Set_Value_From_String( A_messageID,
                                       A_tag,
                                       A_value );
    if ( status != MC_NORMAL_COMPLETION )
    {
        MC_Get_Tag_Info(A_tag, tagDescription, sizeof(tagDescription));
        LogError( ERROR_LVL_1, A_function, __LINE__, status,
                  "Failed to set the tag %s to \"%s\"\n",
                  tagDescription, A_value );
        return ( FAILURE );
    }
    
    return ( SUCCESS );
}



/*****************************************************************************
**
** NAME
**    SetAndSendWorklistMsg
**
** SYNOPSIS
**    static int SetAndSendWorklistMsg
**
** ARGUMENTS
**    int  A_associationID - id of the association
**    char *A_modality     - the modality name
**    char *A_aeTitle      - ae title
**    char *A_startDate    - start date 
**    char *A_patientName  - patient's name
**    char *A_patientID    - patient's ID
**    char *A_scheduledProcStepID    - Scheduled Procedure Step ID
**
** DESCRIPTION
**    Opens a message, fills in some of the "required" fields, and then
**    sends the message off to the service class provider.
**
** RETURNS
**
** SEE ALSO
**    
*****************************************************************************/
static int
SetAndSendWorklistMsg(
                         int  A_associationID,
                         char *A_modality,
                         char *A_aeTitle,
                         char *A_startDate,
                         char *A_patientName,
                         char *A_patientID,
                         char *A_scheduledProcStepID
                     )

{
    MC_STATUS    status;
    int          messageID;
    int          procedureStepItemID;
    char         nullString[ PN_LEN ];

    nullString[0]='\0';
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
        LogError( ERROR_LVL_1, "SetAndSendWorklistMsg", __LINE__, status,
                  "Unable to open a message for MODALITY_WORKLIST_FIND\n" );
        return ( FAILURE );
    }


    /*
    ** Some of the values that we want to query on are part of a sequence
    ** within the FIND.  Therefore, we attempt to open an item...
    */
    status = MC_Open_Item( &procedureStepItemID,
                           "SCHEDULED_PROCEDURE_STEP" );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SetAndSendWorklistMsg", __LINE__, status,
                  "Unable to open an item for "
                  "SCHEDULED_PROCEDURE_STEP\n" );
        MC_Free_Message( &messageID );
        return ( FAILURE );
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
        LogError( ERROR_LVL_1, "SetAndSendWorklistMsg", __LINE__, status,
                  "Unable to set value for MSG_ID:  %d\n"
                  "                           TAG:  %lx\n"
                  "                     TAG VALUE:  %s\n",
                  messageID,
                  MC_ATT_SCHEDULED_PROCEDURE_STEP_SEQUENCE,
                  procedureStepItemID );
        MC_Free_Item( &procedureStepItemID );
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    /*
    ** In order to setup the message, we use the MC_Set_Value... functions.
    ** We also include some fields that are part of the IHE requirements.
    ** We set those values to NULL, indicitating that the SCP should give
    ** them to us if it can.
    */

    if ( SetNullOrStringValue( procedureStepItemID, 
                               MC_ATT_MODALITY, 
                               A_modality, 
                               "SetAndSendWorklistMsg" ) != SUCCESS ) 
    {
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    /*
    ** If any of the required fields are still unfilled, then we will
    ** set them to NULL, lest we get invalid tag messages.
    */
    if ( SetNullOrStringValue( procedureStepItemID, 
                        MC_ATT_SCHEDULED_STATION_AE_TITLE,
                        A_aeTitle, 
                        "SetAndSendWorklistMsg" ) != SUCCESS ) 
    {
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    /*
    ** If any of the required fields are still unfilled, then we will
    ** default them, lest we get invalid tag messages.
    */
    if ( SetNullOrStringValue( procedureStepItemID, 
                        MC_ATT_SCHEDULED_PROCEDURE_STEP_START_DATE,
                        A_startDate, 
                        "SetAndSendWorklistMsg" ) != SUCCESS ) 
    {
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    /*
    ** The Scheduled Procedure Step ID tag is used in the second query to
    ** guarentee uniqueness.  Set it properly here if supplied.
    */
    if ( SetNullOrStringValue( procedureStepItemID, 
                        MC_ATT_SCHEDULED_PROCEDURE_STEP_ID,
                        A_scheduledProcStepID, 
                        "SetAndSendWorklistMsg" ) != SUCCESS ) 
    {
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    /*
    ** OK.  We are though with setting the required fields with their entered
    ** data or a wild card.  Now we must "NULL out" all of the other fields
    ** that the server is going to look at.
    */

    /*
    ** We don't allow the user to set a value for the time.  Since we don't
    ** allow them to enter a time, we give the SCP a universal match for
    ** the entire day.
    */
    status = MC_Set_Value_To_NULL( procedureStepItemID,
                                 MC_ATT_SCHEDULED_PROCEDURE_STEP_START_TIME );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SetAndSendWorklistMsg", __LINE__, status,
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "                                TAG:  %lx\n"
                  "                          TAG VALUE:  %s\n",
                  procedureStepItemID,
                  MC_ATT_SCHEDULED_PROCEDURE_STEP_START_TIME,
                  "NULL" );
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    /*
    ** We don't allow query by physician's name, so we will blank it out.
    */
    status = MC_Set_Value_To_NULL( procedureStepItemID,
                                 MC_ATT_SCHEDULED_PERFORMING_PHYSICIANS_NAME );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SetAndSendWorklistMsg", __LINE__, status,
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "                                TAG:  %lx\n"
                  "                          TAG VALUE:  %s\n",
                  procedureStepItemID,
                  MC_ATT_SCHEDULED_PERFORMING_PHYSICIANS_NAME,
                  "NULL" );
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }


    /*
    ** Again, if the requested procedure id isn't set, we will set it
    ** to the NULL value.
    */
    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_REQUESTED_PROCEDURE_ID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SetAndSendWorklistMsg", __LINE__, status,
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "                                TAG:  %lx\n"
                  "                          TAG VALUE:  %s\n",
                  messageID,
                  MC_ATT_REQUESTED_PROCEDURE_ID,
                  "NULL" );
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    /*
    ** Requested procedure description...
    */
    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_REQUESTED_PROCEDURE_DESCRIPTION );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SetAndSendWorklistMsg", __LINE__, status,
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "                                TAG:  %lx\n"
                  "                          TAG VALUE:  %s\n",
                  messageID,
                  MC_ATT_REQUESTED_PROCEDURE_DESCRIPTION,
                  "NULL" );
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    /*
    ** Again, if the study instance uid isn't set, we will set it
    ** to the NULL value.
    */
    status = MC_Set_Value_To_NULL( messageID,
                                   MC_ATT_STUDY_INSTANCE_UID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SetAndSendWorklistMsg", __LINE__, status,
                  "Unable to set NULL value for MSG_ID:  %d\n"
                  "                                TAG:  %lx\n"
                  "                          TAG VALUE:  %s\n",
                  messageID,
                  MC_ATT_STUDY_INSTANCE_UID,
                  "NULL" );
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    /*
    ** Again, if the patient name isn't set, we will set it
    ** to the NULL value.
    */
    if ( SetNullOrStringValue( messageID, 
                        MC_ATT_PATIENTS_NAME,
                        A_patientName, 
                        "SetAndSendWorklistMsg" ) != SUCCESS ) 
    {
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    /*
    ** Again, if the patient id isn't set, we will set it
    ** to the NULL value.
    */
    if ( SetNullOrStringValue( messageID, 
                        MC_ATT_PATIENT_ID,
                        A_patientID, 
                        "SetAndSendWorklistMsg" ) != SUCCESS ) 
    {
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( messageID, 
                        MC_ATT_PATIENTS_BIRTH_DATE,
                        nullString, 
                        "SetAndSendWorklistMsg" ) != SUCCESS ) 
    {
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( messageID, 
                        MC_ATT_PATIENTS_SEX,
                        nullString,
                        "SetAndSendWorklistMsg" ) != SUCCESS ) 
    {
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( messageID, 
                        MC_ATT_REFERRING_PHYSICIANS_NAME,
                        nullString,
                        "SetAndSendWorklistMsg" ) != SUCCESS ) 
    {
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( messageID, 
                        MC_ATT_ACCESSION_NUMBER,
                        nullString,
                        "SetAndSendWorklistMsg" ) != SUCCESS ) 
    {
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( procedureStepItemID, 
                        MC_ATT_SCHEDULED_PROCEDURE_STEP_DESCRIPTION,
                        nullString,
                        "SetAndSendWorklistMsg" ) != SUCCESS ) 
    {
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( messageID, 
                        MC_ATT_REQUESTED_PROCEDURE_DESCRIPTION,
                        nullString,
                        "SetAndSendWorklistMsg" ) != SUCCESS ) 
    {
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( messageID, 
                        MC_ATT_REFERENCED_STUDY_SEQUENCE,
                        nullString,
                        "SetAndSendWorklistMsg" ) != SUCCESS ) 
    {
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( procedureStepItemID, 
                        MC_ATT_SCHEDULED_ACTION_ITEM_CODE_SEQUENCE,
                        nullString,
                        "SetAndSendWorklistMsg" ) != SUCCESS ) 
    {
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( messageID, 
                        MC_ATT_REQUESTED_PROCEDURE_CODE_SEQUENCE,
                        nullString,
                        "SetAndSendWorklistMsg" ) != SUCCESS ) 
    {
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }


    /*
     * Validate and list the message
     */
    ValMessage      (messageID, "C-FIND-RQ");
    WriteToListFile (messageID, "cfindrq.msg");

    /*
    ** Once the message has been built, we are then able to perform the
    ** C-FIND on it.
    */
    status = MC_Send_Request_Message( A_associationID, messageID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SetAndSendWorklistMessage", __LINE__, status,
                  "Unable to send the request message to the SCP.\n" );
        MC_Free_Message( &messageID );
        return ( FAILURE );
    }

    /*
    ** After sending the message, we free it.
    */
    status = MC_Free_Message( &messageID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SetAndSendWorklistMessage", __LINE__, status,
                  "Unable to free the request message.\n" );
    }

    
    return( SUCCESS );

} /* End of SetAndSendWorklistMsg */



/*****************************************************************************
**
** NAME
**    ProcessWorklistReplyMsg
**
** SYNOPSIS
**    static int ProcessWorklistReplyMsg
**
** ARGUMENTS
**    int   A_associationID - the association
**    LIST *A_list          - a pointer to the worklist's list
**
** DESCRIPTION
**    Waits for a reply message(s) from the service class provider.
**    Once the server sends a message, we then want to process the
**    message.  In this test program, we process the message by just
**    printing out the contents of the response.
**
** RETURNS
**    SUCCESS or FAILURE
**
** SEE ALSO
**    
*****************************************************************************/
static int
ProcessWorklistReplyMsg(
                           int   A_associationID,
                           LIST *A_list
                       )
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
    WORK_PATIENT_REC patientRec;       /* The data from the patient        */
    int   itemNum;                     /* The item number that pertains to */
                                       /* the sequence of procedure steps  */
    int   messageCount = 0;            /* The number of responses from SCP */
    int   alreadySent = FALSE;

    /*
    ** Until finished,  we'll loop, looking for messages from the server.
    */
    while( TRUE )
    {
        /*
        ** Read a message from the formed association, timing out in
        ** G_timeout seconds.
        */
        status = MC_Read_Message( A_associationID, G_timeout,
                                  &responseMessageID,
                                  &serviceName, &command );
        if ( status != MC_NORMAL_COMPLETION )
        {
            LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg", __LINE__, status,
                      "Error reading the message from the SCP.\n" );
            return( FAILURE );
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
            LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg", __LINE__, status,
                      "Error checking the response from the SCP.\n" );
            return( FAILURE );
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
                LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg",
                          __LINE__, status,
                          "Error freeing message:  %d\n", responseMessageID );
                return( FAILURE );
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
            ** Here is where we actually grab the data from the toolkit's
            ** data strctures and attempt to display it to the user.
            */

            /*
            ** Since some of the responses are contained within a sequence,
            ** we will attempt to open that sequence as an item:
            */
            status = MC_Get_Value_To_Int( responseMessageID,
                                       MC_ATT_SCHEDULED_PROCEDURE_STEP_SEQUENCE,
                                          &itemNum );
            if ( status != MC_NORMAL_COMPLETION )
            {
                LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg",
                          __LINE__, status,
                          "Error attempting to read the scheduled procedure "
                          "step sequence id.\n" );
                MC_Free_Message( &responseMessageID );
                return( FAILURE );
            }
      
            /*
            ** If we were given a start date, then fill it in.
            */
            status = MC_Get_Value_To_String( itemNum,
                                    MC_ATT_SCHEDULED_PROCEDURE_STEP_START_DATE,
                                             sizeof( patientRec.startDate ),
                                             patientRec.startDate );
            if ( status != MC_NORMAL_COMPLETION )
            {
                LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg",
                          __LINE__, status,
                          "Error attempting to read the procedure start\n"
                          "date.\n" );
                MC_Free_Message( &responseMessageID );
                return( FAILURE );
            }
          
            /*
            ** If we were given a start time, then fill it in.
            */
            status = MC_Get_Value_To_String( itemNum,
                                    MC_ATT_SCHEDULED_PROCEDURE_STEP_START_TIME,
                                             sizeof( patientRec.startTime ),
                                             patientRec.startTime );
            if ( status != MC_NORMAL_COMPLETION )
            {
                LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg",
                          __LINE__, status,
                          "Error attempting to read the procedure start\n"
                          "time.\n" );
                MC_Free_Message( &responseMessageID );
                return( FAILURE );
            }

            /*
            **  When given a modality, fill it in.
            */
            status = MC_Get_Value_To_String( itemNum,
                                             MC_ATT_MODALITY,
                                             sizeof( patientRec.modality ),
                                             patientRec.modality );
            if ( status != MC_NORMAL_COMPLETION )
            {
                LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg", 
                          __LINE__, status,
                          "Error attempting to read the modality.\n" );
                MC_Free_Message( &responseMessageID );
                return( FAILURE );
            }

            /*
            ** Now we have a patient's name, so fill it in.
            */
            status = MC_Get_Value_To_String( itemNum,
                                   MC_ATT_SCHEDULED_PERFORMING_PHYSICIANS_NAME,
                                             sizeof( patientRec.physicianName ),
                                             patientRec.physicianName );
            if (( status != MC_NORMAL_COMPLETION )
             && ( status != MC_NULL_VALUE ))
            {
                LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg",
                          __LINE__, status,
                          "Error attempting to read the response message \n"
                          "id.\n" );
                MC_Free_Message( &responseMessageID );
                return( FAILURE );
            }

            /*
            ** When given a scheduled procedure step, we want to fill
            ** that in as well.
            */
            status = MC_Get_Value_To_String( itemNum,
                                   MC_ATT_SCHEDULED_PROCEDURE_STEP_ID,
                                             sizeof( patientRec.stepID ),
                                             patientRec.stepID );
            if ( status != MC_NORMAL_COMPLETION )
            {
                LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg",
                          __LINE__, status,
                          "Error attempting to read the scheduled "
                          "procedure step id.\n" );
                MC_Free_Message( &responseMessageID );
                return( FAILURE );
            }

            /*
            ** When given a scheduled procedure step description, we want
            ** to fill that in as well.
            */
            status = MC_Get_Value_To_String( itemNum,
                                   MC_ATT_SCHEDULED_PROCEDURE_STEP_DESCRIPTION,
                                             sizeof( patientRec.stepDesc ),
                                             patientRec.stepDesc );
            if ( status != MC_NORMAL_COMPLETION )
            {
                LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg",
                          __LINE__, status,
                          "Error attempting to read the scheduled "
                          "procedure step description.\n" );
                MC_Free_Message( &responseMessageID );
                return( FAILURE );
            }

            /*
            ** If given a requested procedure id, then we read it in.
            */
            status = MC_Get_Value_To_String( responseMessageID,
                                             MC_ATT_REQUESTED_PROCEDURE_ID,
                                             sizeof( patientRec.procedureID ),
                                             patientRec.procedureID );
            if ( status != MC_NORMAL_COMPLETION )
            {
                LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg",
                          __LINE__, status,
                          "Error attempting to read the requested \n"
                          "procedure id.\n" );
                MC_Free_Message( &responseMessageID );
                return( FAILURE );
            }

            /*
            ** If given a requested procedure description, then we read it in.
            */
            status = MC_Get_Value_To_String( responseMessageID,
                                        MC_ATT_REQUESTED_PROCEDURE_DESCRIPTION,
                                             sizeof( patientRec.procedureDesc ),
                                             patientRec.procedureDesc );
            if ( status != MC_NORMAL_COMPLETION )
            {
                LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg",
                          __LINE__, status,
                          "Error attempting to read the requested \n"
                          "procedure description.\n" );
                MC_Free_Message( &responseMessageID );
                return( FAILURE );
            }

            /*
            ** If we have a study instance UID, then fill it in.
            */
            status = MC_Get_Value_To_String( responseMessageID,
                                             MC_ATT_STUDY_INSTANCE_UID,
                                             sizeof( patientRec.studyInstance ),
                                             patientRec.studyInstance );
            if ( status != MC_NORMAL_COMPLETION )
            {
                LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg",
                          __LINE__, status,
                          "Error attempting to read the study instance\n"
                          "UID.\n" );
                MC_Free_Message( &responseMessageID );
                return( FAILURE );
            }

            /*
            ** If we have an accession number, then fill it in.
            */
            status = MC_Get_Value_To_String( responseMessageID,
                                             MC_ATT_ACCESSION_NUMBER,
                                             sizeof( patientRec.accession ),
                                             patientRec.accession );
            if (( status != MC_NORMAL_COMPLETION )
             && ( status != MC_NULL_VALUE ))
            {
                LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg",
                          __LINE__, status,
                          "Error attempting to read the accession number."
                          "\n" );
                MC_Free_Message( &responseMessageID );
                return( FAILURE );
            }

            /*
            ** Now we grab the patient's name
            */
            status = MC_Get_Value_To_String( responseMessageID,
                                             MC_ATT_PATIENTS_NAME,
                                             sizeof( patientRec.patientName ),
                                             patientRec.patientName );
            if ( status != MC_NORMAL_COMPLETION )
            {
                LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg",
                          __LINE__, status,
                          "Error attempting to read the patient's name.\n" );
                MC_Free_Message( &responseMessageID );
                return( FAILURE );
            }

            status = MC_Get_Value_To_String( responseMessageID,
                                             MC_ATT_PATIENT_ID,
                                             sizeof( patientRec.patientID ),
                                             patientRec.patientID );
            if ( status != MC_NORMAL_COMPLETION )
            {
                LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg",
                          __LINE__, status,
                          "Error attempting to read the patient id.\n" );
                MC_Free_Message( &responseMessageID );
                return( FAILURE );
            }

            /*
            ** Patient birth date.
            */
            status = MC_Get_Value_To_String( responseMessageID,
                                             MC_ATT_PATIENTS_BIRTH_DATE,
                                          sizeof( patientRec.patientBirthDate ),
                                             patientRec.patientBirthDate );
            if ( status != MC_NORMAL_COMPLETION )
            {
                LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg",
                          __LINE__, status,
                          "Error attempting to read the patient birth "
                          "date.\n" );
                MC_Free_Message( &responseMessageID );
                return( FAILURE );
            }

            /*
            ** Patient sex.
            */
            status = MC_Get_Value_To_String( responseMessageID,
                                             MC_ATT_PATIENTS_SEX,
                                             sizeof( patientRec.patientSex ),
                                             patientRec.patientSex );
            if ( status != MC_NORMAL_COMPLETION )
            {
                LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg",
                          __LINE__, status,
                          "Error attempting to read the patient sex."
                          "\n" );
                MC_Free_Message( &responseMessageID );
                return( FAILURE );
            }


            /*
            ** Attempt to free the message, and then wait for the next one
            ** from the server.
            */
            status = MC_Free_Message( &responseMessageID );
            if ( status != MC_NORMAL_COMPLETION )
            {
                LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg",
                          __LINE__, status,
                          "Error attempting to free message:   %d\n",
                          responseMessageID );
                LLInsert( A_list, &patientRec );
                return( FAILURE );
            }

            /*
            ** The above "..Value_To_String" calls grab the actual responses
            ** from the server.  We took these responses and stuffed them into
            ** a structure.  At this point, a real application would
            ** take over and attempt to process the information that it had
            ** requested and received.  We push them onto a list for future
            ** usage...
            */
            if ( LLInsert( A_list, &patientRec ) != SUCCESS )
            {
                return( FAILURE );
            }


            /*
            ** We've read a message.  If we've read more messages than
            ** what we are looking for, then we want to send a
            ** C_CANCEL_RQ back to the SCP.
            */
            messageCount++;
            if ( ( messageCount >= G_threshold ) &&
                 ( alreadySent != TRUE ) )
            {
                /*
                ** We use these two variables to make sure that we
                ** only send ONE cancel message to the SCP.
                */
                messageCount = 0;
                alreadySent = TRUE;

                if ( SendCcancelRQ( A_associationID ) == FAILURE )
                {
                    return( FAILURE );
                }
            }
    
        }
        else if( response == C_FIND_CANCEL_REQUEST_RECEIVED )
        {
          /*
          ** In this case, the SCP is acknowledging our sending of a
          ** request to cancel the query.
          */
          printf( "Received acknowledgement of a C_CANCEL_RQ message...\n" );

          /*
          ** Attempt to free the message, and then wait for the next one
          ** from the server.
          */
          status = MC_Free_Message( &responseMessageID );
          if ( status != MC_NORMAL_COMPLETION )
          {
              LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg", __LINE__,
              status,
                        "Error attempting to free message:   %d\n",
                        responseMessageID );
              return( FAILURE );
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
            LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg",
                      __LINE__, MC_NORMAL_COMPLETION,
                      "Obtained an unknown response message from the SCP.\n"
                      "response:  %X.=n", response );
            MC_List_Message( responseMessageID, NULL );
            status = MC_Free_Message( &responseMessageID );
            if ( status != MC_NORMAL_COMPLETION )
            {
                LogError( ERROR_LVL_1, "ProcessWorklistReplyMsg",
                          __LINE__, status,
                          "Error freeing message:  %d\n",
                          responseMessageID );
                return ( FAILURE );
            }
            break; /* nothing should come after this... */

        } /* End of giant if-then-else */

    } /* End of infinite loop */

    return( SUCCESS );

} /* End of ProcessWorklistReplyMsg */


/*****************************************************************************
**
** NAME
**    SendCcancelRQ
**
** SYNOPSIS
**    static int SendCcancelRQ
**
** ARGUMENTS
**    int  A_associationID - the association ID
**
** DESCRIPTION
**    This function will send a C_CANCEL_RQ message to the SCP.  It is
**    used to tell the SCP that we've received enough, or too much, data
**    and that it should stop sending data to us.
**
** RETURNS
**    SUCCESS or FAILURE
**
** SEE ALSO
**    
*****************************************************************************/
static int
SendCcancelRQ(
                 int  A_associationID
             )
{
    int       messageID;
    MC_STATUS status;

    status = MC_Open_Message( &messageID, "MODALITY_WORKLIST_FIND",
                              C_CANCEL_RQ );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendCcancelRQ", __LINE__, status,
                  "Unable to open a message for MODALITY_WORKLIST_FIND\n" );
        return ( FAILURE );
    }
    
    printf( "Sending a C_CANCEL_RQ...\n" );

    status = MC_Send_Request_Message( A_associationID, messageID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        MC_Free_Message ( &messageID );
        LogError( ERROR_LVL_1, "SendCcancelRQ", __LINE__, status,
                  "Unable to send the cancel message to the SCP.\n" );
        return ( FAILURE );
    }
    
    status = MC_Free_Message( &messageID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendCcancelRQ", __LINE__, status,
                  "Error attempting to free message:   %d\n",
                  messageID );
        return( FAILURE );
    }

    return( SUCCESS );
}


/*****************************************************************************
**
** NAME
**    GatherMoreData
**
** SYNOPSIS
**   static int GatherMoreData
**
** ARGUMENTS
**   int   A_applicationID - the application ID
**   LIST *A_list          - a pointer to a list of matching patients
**   char *A_modality      - a modality
**   char *A_ae_title      - an AE title
**   char *A_startDate     - the start date
**
** DESCRIPTION
**    This is the second sub-menu, that takes the list of patients that was
**    obtained earlier, and then asks the user to select one.  After selecting
**    a specific patient, another C_FIND_REQUEST is performed.  More detailed
**    information is displayed to the user.
**    It is a simple addition to this sub-menu to implement performed
**    procedure step, once it gains approval.
**
** RETURNS
**    SUCCESS or FAILURE
**
** SEE ALSO
**    
*****************************************************************************/
static int
GatherMoreData(
                  int   A_applicationID,
                  LIST *A_list,
                  char *A_modality,
                  char *A_ae_title,
                  char *A_startDate
              )
{
    int         associationID;
    int         count = 0;
    char        typing[ INPUT_LEN ];
    char        junk[ INPUT_LEN ];
    int         entered;
    int         goodNumber = FALSE;
    int         didExit = FALSE;
    WORK_PATIENT_REC patient;
    LIST        responseList;
    MC_STATUS   status;

    /*
    ** This list is used to hold the patient data that is send back to us
    ** from the server in our second C_FIND request.
    */
    if( LLCreate( &responseList, sizeof( WORK_PATIENT_REC ) ) != SUCCESS )
    {
        LogError( ERROR_LVL_1, "GatherMoreData", __LINE__, MC_NORMAL_COMPLETION,
                  "Unable to initialize the linked list.\n" );
        return( FAILURE );
    }

    while( goodNumber == FALSE )
    {
        printf( "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n" );
        printf( "      %16.16s %16.16s\n", "Patient Name", "Patient ID" );
        printf( "      %16.16s %16.16s\n", "------------", "----------" );

        /*
        ** Rewind the list of patients so that we can display the
        ** lists contents.
        */
        LLRewind( A_list );
    
        /*
        ** Now, walk the list, getting each record from it.
        */
        while( LLPopNode( A_list, &patient ) != NULL )
        {
            count++;
            printf( "[%2d]  %16.16s %16.16s\n", count, patient.patientName,
                     patient.patientID );
        }

        printf( "Select a patient to start a Modality Performed Procedure " );
        printf( "Step for,\nor 'x' to exit:\n" );
        printf( "==>" );
        fgets( typing, sizeof( typing ), stdin );
        if( ( typing[0] == 'X' ) || ( typing[0] =='x' ) )
        {
            didExit = TRUE;
            goodNumber = TRUE;
        }
        entered = strtoul( typing, NULL, 10 );
        if ( ( ( entered > count ) || ( entered < 1 ) ) &&
               ( didExit != TRUE ) )
        {
            printf( "'%c' is invalid.\n", typing[0] );
            count = 0;
            printf( "Press <return> to continue.\n" );
            fgets( junk, sizeof( junk ), stdin );
        }
        else
        {
            /*
            ** They entered a "good" number.
            */
            goodNumber = TRUE;
        }

    } /* end of while goodNumber == false */
 
    /*
    ** If the user chose to exit, then we just return.  We don't want to
    ** start a MPPS operation if they don't want to.
    */
    if ( didExit == TRUE )
    {
        return( SUCCESS );
    }

    /*
    ** Get the patient information associated with the number that
    ** they entered.
    */
    LLPopNodeN( A_list, &patient, entered );

    /*
    ** Attempt to open an association with the provider.
    */
    if ( OpenAssociation( A_applicationID, &associationID ) != SUCCESS )
    {
        LogError( ERROR_LVL_1, "GatherMoreData", __LINE__, MC_NORMAL_COMPLETION,
                  "Unable to open the association with the SCP.\n" );
        LLDestroy( &responseList );
        return( FAILURE );
    }

    /*
    ** Create and send another C-FIND message.
    */
    if ( SetAndSendWorklistMsg( associationID, A_modality, A_ae_title,
                                A_startDate, patient.patientName,
                                patient.patientID, patient.stepID ) == FAILURE )
    {
        LLDestroy( &responseList );
        return( FAILURE );
    }

    /*
    ** Wait for, and process another C-FIND reply message.
    */
    if ( ProcessWorklistReplyMsg( associationID,
                                  &responseList ) == FAILURE )
    {
        LLDestroy( &responseList );
        return( FAILURE );
    }

    /*
    ** Now that we are done with the SCP, we'll close the association that
    ** we've formed with the server.
    */
    status = MC_Close_Association( &associationID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "GatherMoreData", __LINE__, MC_NORMAL_COMPLETION,
                  "Unable to close the assiciation with MERGE_WORK_SCP.\n" );
        LLDestroy( &responseList );
        return( FAILURE );
    }

    /*
    ** We must look to see how many responses we got back from the worklist
    ** SCP.  We should only get one.  Should we get none, then the patient
    ** whom we've obtained via the initial worklist query is no longer
    ** available to us (perhaps another modality is already working on that
    ** patient).
    */
    if ( LLNodes( &responseList ) == 0 )
    {
        printf( "\nUnable to obtain patient ID:  %s\n",
                patient.patientID );
        printf( "from the worklist SCP.\n" );
        printf( "Perhaps the patient is no longer available.\n" );
        printf( "Please try again.\n\n" );
    }
    else if ( LLNodes( &responseList ) > 1 )
    {
        /*
        ** This sample application can only handle a single matching
        ** patient based on the initial query criteria.  Of course,
        ** a real application MUST implement this possibility, and NOT
        ** with an error message like is being done here!!!!!
        */
        printf( "\nFound more that one patient matching patient ID:  %s\n",
                patient.patientID );
        printf( "from the worklist SCP.\n" );
        printf( "This sample application can only handle a single reply.\n" );
        printf( "You must requery from the initial query options.\n\n" );
    }
    else
    {
        /*
        ** There should be only one node on the list, since we've queried by
        ** key values:  patient name and patient id.  That is why we pop only
        ** the first value off of the list and display those results to the
        ** user.
        */
        LLPopNodeN( &responseList, &patient, 1 );

        /*
        ** At this point, we go into the Modality Performed Procedure Step
        ** functionality.
        ** We consider this choice to be the start of the MPPS.  That is why
        ** you can exit from this menu without choosing a patient.   
        */
        PerformMPPS( &patient, A_applicationID );
    }

    printf( "\npress <return> to continue\n" );
    fgets( junk, sizeof( junk ), stdin );

    /*
    ** Delete the response list
    */
    LLDestroy( &responseList );
    return( SUCCESS );
}



/*****************************************************************************
**
** NAME
**    PerformMPPS
**
** SYNOPSIS
**    static void PerformMPPS
**
** ARGUMENTS
**    WORK_PATIENT_REC *A_patient       - a pointer to a single patient rec
**    int               A_applicationID - the application ID
**
** DESCRIPTION
**    This is the third sub-menu.  After identifying a patient via Modality
**    worklist, and then verifying that a patient actually exists, this
**    function sends the initial N_CREATE message to the procedure step SCP,
**    and then gives the user the ability to do some modality imaging
**    functionality.
**
** RETURNS
**    nothing
**
** SEE ALSO
**    
*****************************************************************************/
static void
PerformMPPS(
               WORK_PATIENT_REC *A_patient,
               int               A_applicationID
           )
{
    int         goodNumber = FALSE;
    int         didExit = FALSE;
    char        junk[ INPUT_LEN ];
    char        typing[ INPUT_LEN ];
    int         entered;
    int         acqStarted = FALSE;
    int         acqCompleted = FALSE;
    char        affectedSOPinstance[UI_LEN];

    /*
    ** Another sub-menu...  This time, we simulate an actual modality
    ** in which we do image acquision and relay performed procedure step
    ** information to an SCP.
    */
    while( goodNumber == FALSE )
    {
        printf( "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n" );
        printf( "Scheduled patient:\n" );
        printf( "%16.16s     %16.16s\n", "Patient Name", "Patient ID" );
        printf( "%16.16s     %16.16s\n", A_patient->patientName,
                A_patient->patientID );
        printf( "\n" );
        printf( "%16.16s     %16.16s\n", "Study Date", "Study Time" );
        printf( "%16.16s     %16.16s\n", A_patient->startDate,
                                         A_patient->startTime );
        printf( "\n" );
        printf( "%16.16s     %18.18s\n", "Modality", "Procedure Step ID" );
        printf( "%16.16s     %18.18s\n", A_patient->modality,
                                         A_patient->stepID );
        printf( "\n" );
        printf( "%16.16s     %16.16s\n", "Physicians Name", "Procedure ID" );
        printf( "%16.16s     %16.16s\n", A_patient->physicianName,
                                         A_patient->procedureID );
        printf( "\n" );
        printf( "%16.16s\n", "Study Instance" );
        printf( "  %-64.64s\n", A_patient->studyInstance );
        printf( "\n** MODALITY PERFORMED PROCEDURE STEP **\n" );
        printf( "\n" );
        printf( "[1] Start image acquisition (N_CREATE MPPS)\n" );
        printf( "[2] Image w/o complete      (N_SET MPPS w/ image info)\n" );
        printf( "[3] Complete and exit MPPS  (N_SET MPPS status=COMPLETED)\n" );
        printf( "[X] Exit, without starting MPPS\n" );

        printf( "==>" );
        fgets( typing, sizeof( typing ), stdin );
        if( ( typing[0] == 'X' ) || ( typing[0] =='x' ) )
        {
            /* 
            ** Before break'ing, we need to verify if acquisition was started
            ** but not completed.
            */
            if ( ( acqStarted == TRUE ) && ( acqCompleted != TRUE ) )
            {
                printf( "\nYou cannot start a MPPS instance without\n" );
                printf( "finishing it.  Please complete the image\n" );
                printf( "acquisition before attempting to exit this\n" );
                printf( "sub-menu.\n\n" );
                didExit = FALSE;
                goodNumber = FALSE;
                printf( "Press <return> to continue.\n" );
                fgets( junk, sizeof( junk ), stdin );
                continue;
            }
            else
            {
                didExit = TRUE;
                goodNumber = TRUE;
                break;
            }
        }
        entered = strtoul( typing, NULL, 10 );
        if ( ( ( entered > 3 ) || ( entered < 1 ) ) &&
               ( didExit != TRUE ) )
        {
            printf( "'%c' is invalid.\n", typing[0] );
            printf( "Press <return> to continue.\n" );
            fgets( junk, sizeof( junk ), stdin );
        }
        else
        {
            /*
            ** They entered a "good" number.
            */
            goodNumber = TRUE;
        }

        /*
        ** Based on what the user entered, we send out modality performed
        ** precedure step messages.
        */
        switch( entered )
        {
            case 1:
                /*
                ** The user chose to simulate image acquisition.
                */
                if ( acqStarted != TRUE )
                {
                    printf( "Starting image acquisition...\n" );
                    if ( SendNCREATERQ( A_applicationID, 
                                       affectedSOPinstance,
                                       A_patient ) != SUCCESS )
                    {
                        printf( "Failed to successfully send an NCREATE " );
                        printf( "request message to the SCP and receive\n" );
                        printf( "a valid response back from the SCP.  " );
                        printf( "Cannot start image acquisition.\n" );
                        acqStarted = FALSE;
                    }
                    else
                    {
                        acqStarted = TRUE;
                    }
                }
                else
                {
                    printf( "Since starting image acquisition on the above\n" );
                    printf( "patient has already been accomplished, you\n" );
                    printf( "cannot perform this function again on the\n" );
                    printf( "same patient.\n" );
                }
                goodNumber = FALSE;
                printf( "Press <return> to continue.\n" );
                fgets( junk, sizeof( junk ), stdin );
                break;
            case 2:
                /*
                ** The user chose to update patient information.
                */
                if ( acqStarted == TRUE )
                {
                    printf( "Sending N_SET RQ w/o status...\n" );
                    SendNSETRQ( A_applicationID,
                                 affectedSOPinstance );
                    goodNumber = FALSE;
                    printf( "Press <return> to continue.\n" );
                    fgets( junk, sizeof( junk ), stdin );
                    break;
                }
                else
                {
                    printf( "Cannot update patient without first starting " );
                    printf( "image acquisition.\n" );
                    goodNumber = FALSE;
                    printf( "Press <return> to continue.\n" );
                    fgets( junk, sizeof( junk ), stdin );
                    break;
                }
            case 3:
                /*
                ** The user chose to complete this procedure step.
                */
                if ( acqStarted == FALSE )
                {
                    printf( "You cannot complete image acquisition without\n" );
                    printf( "having first started it.\n" );
                    goodNumber = FALSE;
                    printf( "Press <return> to continue.\n" );
                    fgets( junk, sizeof( junk ), stdin );
                    break;
                }
                else
                {
                    printf( "Sending N_SET RQ status=COMPLETED...\n" );
                    SendNSETRQComplete( A_applicationID, affectedSOPinstance,
                                        A_patient );
                    acqCompleted = TRUE;
                    break;
                }
    
        } /* end of switch */
    } /* end of while goodNumber == false */
}


/*****************************************************************************
**
** NAME
**    SendNCREATERQ
**
** SYNOPSIS
**    static int SendNCREATERQ(
**
** ARGUMENTS
**    int               A_applicationID       - the application ID
**    char             *A_affectedSOPinstance - the affected SOP instance
**    WORK_PATIENT_REC *A_patient             - a pointer to a single patient
**
** DESCRIPTION
**    This function simulates the acquision of an image by an imaging
**    modality.  As a consequence of that performed procedure, information
**    regarding a performed procedure step is relayed to the worklist/mpps
**    SCP.
**
** RETURNS
**    SUCCESS or FAILURE
**
** SEE ALSO
**    
*****************************************************************************/
static int
SendNCREATERQ( 
                 int               A_applicationID,
                 char             *A_affectedSOPinstance,
                 WORK_PATIENT_REC *A_patient
             )
{
    MC_STATUS    status;
    int          associationID;
    int          messageID;
    int          rspMsgID;
    unsigned int response;
    char         *rspSrvName;
    MC_COMMAND   rspCommand;
    char         mppsStatus[ CS_LEN ];

    /*
    ** In order to send the PERFORMED PROCEDURE STEP N_CREATE message to the
    ** SCP, we need to open a message.
    */
    status = MC_Open_Message( &messageID, "PERFORMED_PROCEDURE_STEP",
                              N_CREATE_RQ );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNCREATERQ", __LINE__, status,
                  "Unable to open a message for PERFORMED_PROCEDURE_STEP\n" );
        return ( FAILURE );
    }

    /*
    ** Once we have an open message, we will then attempt to fill in the
    ** contents of the message.  This is done in a separate funtion.
    */
    if ( SetNCREATERQ( messageID, A_patient ) == FAILURE )
    {
        /*
        ** The error message was logged in the function, itself.  We still
        ** need to free the message.
        */
        MC_Free_Message( &messageID );
        return( FAILURE );
    }

    /*
    ** Now that we've created our request message, we want to open the 
    ** association with an SCP that is providing MPPS.
    */
    if ( OpenAssociation( A_applicationID, &associationID ) != SUCCESS )
    {
        MC_Free_Message( &messageID );
        return( FAILURE );
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
        LogError( ERROR_LVL_1, "SendNCREATERQ", __LINE__, status,
                  "Unable to send the request message to the SCP.\n" );
        MC_Abort_Association( &associationID );
        MC_Free_Message ( &messageID );
        return ( FAILURE );
    }

    /*
    ** Free the request message, since we do not need to use it anymore.
    */
    status = MC_Free_Message( &messageID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNCREATERQ", __LINE__, status,
                  "Unable to free the N_CREATE message.  Fatal error,\n"
                  "this server will exit.\n" );
        MC_Abort_Association( &associationID );
        return( FAILURE );
    }


    /*
    ** We must examine the N_SET response message, especially looking for
    ** the affected SOP instance UID.  This UID will be used to track the
    ** other N_SET functionality later.
    */
    status = MC_Read_Message( associationID, G_timeout,
                              &rspMsgID, &rspSrvName, &rspCommand );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNCREATERQ", __LINE__, status,
                  "Unable to read the response message from the SCP.\n" );
        MC_Abort_Association( &associationID );
        return ( FAILURE );
    }

    /*
    ** And then check the status of the response message.
    ** It had better be N_SET_SUCCESS or else we will exit.  A real application
    ** will probably choose something better than just exiting, though.
    */
    status = MC_Get_Value_To_UInt( rspMsgID, MC_ATT_STATUS, &response );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNCREATERQ", __LINE__, status,
                  "Unable to read the STATUS tag in the response message "
                  "from the SCP.\n" );
        MC_Abort_Association( &associationID );
        return ( FAILURE );
    }
    if ( response != N_CREATE_SUCCESS )
    {
        LogError( ERROR_LVL_1, "SendNCREATERQ", __LINE__, status,
                  "Unexpected response status from the SCP.\n"
                  "Response STATUS was:  %xd\n", response );
        MC_Abort_Association( &associationID );
        return ( FAILURE );
    }

    /*
     * Validate and list the message
     */
    ValMessage      (rspMsgID, "N-CREATE-RSP");
    WriteToListFile (rspMsgID, "ncreatersp.msg");


    /*
    ** Note that we only look for the affected SOP instance UID and STATUS in
    ** the response message.  In reality, a MPPS SCU will probably want to
    ** examine other attributes that are in the response message.  The SCP
    ** will send back an attribute for everyone that was asked for in the
    ** request/create message.
    */
    status = MC_Get_Value_To_String( rspMsgID,
                                     MC_ATT_AFFECTED_SOP_INSTANCE_UID,
                                     UI_LEN,
                                     A_affectedSOPinstance );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNCREATERQ", __LINE__, status,
                  "Failed to read the affected SOP instance UID in the "
                  "N_CREATE_RSP message from the SCP.\n" );
        MC_Abort_Association( &associationID );
        MC_Free_Message( &rspMsgID );
        return ( FAILURE );
    }
    printf( "Received affected SOP instance UID:  %s\n"
            "in N_CREATE response message.\n", A_affectedSOPinstance );

    /*
    ** Although our SCP returns the contents of a message that has been 
    ** set, there is no guanrentee the SCP will do this, so this is not
    ** a failure condition.
    */
    status = MC_Get_Value_To_String( rspMsgID,
                                     MC_ATT_PERFORMED_PROCEDURE_STEP_STATUS,
                                     CS_LEN,
                                     mppsStatus );
    if ( status == MC_NORMAL_COMPLETION )
    {
        printf( "Received performed procedure step status:  %s\n"
                "in N_CREATE response message.\n", mppsStatus );
    }


    /*
    ** ...and then do some cleanup.
    */
    status = MC_Free_Message( &rspMsgID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNCREATERQ", __LINE__, status,
                  "Unable to free the N_CREATE message.  Fatal error,\n"
                  "this server will exit.\n" );
        MC_Abort_Association( &associationID );
        exit( FAILURE );
    }

    /*
    ** Now that we are done, we'll close the association that
    ** we've formed with the server.
    */
    status = MC_Close_Association( &associationID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNCREATERQ", __LINE__, status,
                  "Unable to close the association with "
                  "the MPPS SCP.\n" );
        MC_Abort_Association( &associationID );
        return ( FAILURE );
    }

    return( SUCCESS );
}


/*****************************************************************************
**
** NAME
**    SetNCREATERQ
**
** SYNOPSIS
**    static int SetNCREATERQ
**
** ARGUMENTS
**    int               A_messageID - the N_CREATE message's ID
**    WORK_PATIENT_REC *A_patient   - a pointer to a single patient record
**
** DESCRIPTION
**    This function will set all of the N_CREATE attributes for a message
**    ID that is passed in.  The message ID must be valid, and have been
**    created via:
**
** RETURNS
**    SUCCESS or FAILURE
**
** SEE ALSO
**
*****************************************************************************/
static int
SetNCREATERQ( 
                int               A_messageID,
                WORK_PATIENT_REC *A_patient
            )
{
    MC_STATUS status;
    int       itemID;
    char      date[ 9 ]="";
    char      time[ 7 ]="";
    char      sentUID[ UI_LEN ];

    /*
    ** Here, we setup all of the needed pieces to the performed procedure
    ** step N_CREATE message.  These fields that are listed first are all of
    ** the required fields.  After these, we will fill in some type 2 fields
    ** that tie the whole Modality Worklist and Performed Procedure Step
    ** stuff together.
    **
    **
    ** First, we need an item to hold some of the attributes.
    */
    status = MC_Open_Item( &itemID, "SCHEDULED_STEP_ATTRIBUTE" );

    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNCREATERQ", __LINE__, status,
                  "Failed to open the PROCEDURE STEP "
                  "SEQUENCE item.\n" );
        return ( FAILURE );
    }

    /*
    ** Then we place the reference to this item into the message itself.
    */
    status = MC_Set_Value_From_Int ( A_messageID,
                                     MC_ATT_SCHEDULED_STEP_ATTRIBUTES_SEQUENCE,
                                     itemID );

    if ( status != MC_NORMAL_COMPLETION )
    {
        /*
        ** Since we were unable to set the item ID into the message, we must
        ** free the item ID before we exit.  This is because the item is not
        ** yet tied to the message and freeing the message will not free this
        ** item.
        */
        MC_Free_Item( &itemID );
        LogError( ERROR_LVL_1, "SendNCREATERQ", __LINE__, status,
                  "Failed to set the ITEM ID:  %d\n", itemID );
        return ( FAILURE );
    }

    /*
    ** and then, start filling in some of the attributes that are part of
    ** the item.
    ** First part of the item:  study instance UID.
    */
    if ( SetStringValue ( itemID,
                          MC_ATT_STUDY_INSTANCE_UID,
                          A_patient->studyInstance,
                          "SendNCREATERQ" ) != SUCCESS )
        return ( FAILURE );

    /*
    ** Performed procedure step ID
    */
    if ( SetStringValue ( A_messageID,
                          MC_ATT_PERFORMED_PROCEDURE_STEP_ID,
                          A_patient->procedureID,
                          "SendNCREATERQ" ) != SUCCESS )
        return ( FAILURE );

    if ( SetStringValue ( A_messageID,
                          MC_ATT_PERFORMED_PROCEDURE_STEP_DESCRIPTION,
                          A_patient->procedureDesc,
                          "SendNCREATERQ" ) != SUCCESS )
        return ( FAILURE );
        

    /*
    ** Performed station AE title
    */
    if ( SetStringValue ( A_messageID,
                          MC_ATT_PERFORMED_STATION_AE_TITLE,
                          G_appTitle,
                          "SendNCREATERQ" ) != SUCCESS )
        return ( FAILURE );


    /*
    ** Performed procedure step start date
    ** Both the data and time should be obtained right here.  This happens
    ** because the worklist only contains the scheduled start date and time,
    ** and not the actual start date and time.
    */
    GetCurrentDate( date, time );
    if ( SetStringValue ( A_messageID,
                          MC_ATT_PERFORMED_PROCEDURE_STEP_START_DATE,
                          date,
                          "SendNCREATERQ" ) != SUCCESS )
        return ( FAILURE );

    /*
    ** Performed procedure step start time
    */
    if ( SetStringValue ( A_messageID,
                          MC_ATT_PERFORMED_PROCEDURE_STEP_START_TIME,
                          time,
                          "SendNCREATERQ" ) != SUCCESS )
        return ( FAILURE );


    /*
    ** Performed procedure step status
    */
    if ( SetStringValue ( A_messageID,
                          MC_ATT_PERFORMED_PROCEDURE_STEP_STATUS,
                          "IN PROGRESS",
                          "SendNCREATERQ" ) != SUCCESS )
        return ( FAILURE );


    /*
    ** Modality
    */
    if ( SetStringValue ( A_messageID,
                          MC_ATT_MODALITY,
                          A_patient->modality,
                          "SendNCREATERQ" ) != SUCCESS )
        return ( FAILURE );

    /*
    ** Requested SOP instance UID
    ** This is required by IHE, but not required in general.  If an SCU
    ** doesn't send this value, then the SCP will create the UID and send it
    ** back to us.
    ** In this case, since we sent it, the SCP should send the same UID back
    ** to us.
    */
    strcpy( sentUID, Create_Inst_UID() );
    printf( "Sending requested SOP instance UID:  %s\n", sentUID );
    if ( SetStringValue ( A_messageID,
                          MC_ATT_AFFECTED_SOP_INSTANCE_UID,
                          sentUID,
                          "SendNCREATERQ" ) != SUCCESS )
        return ( FAILURE );


    /*
    ** OK, now we are done with the required fields.  The next few fields
    ** will be used by the MPPS SCP to tie this MPPS N_CREATE request to
    ** a particular patient, and all of that other worklist stuff.
    ** These fields aren't required by a MPPS SCP.  They are type 2.  In cases
    ** where MPPS is used with Modality Worklist, it makes sense to send the
    ** key patient demographic information that was just obtained via the
    ** worklist.  In fact, since our sample worklist SCP is also the MPPS SCP,
    ** the SCP requires these fields in order to "tie" the information together.
    ** If this information isn't sent, then the SCP will create a MPPS instance
    ** that is not tied to any particular patient, and will update your
    ** patient demographic information upon receipt of an N_SET.
    */

    /*
    ** First, set the attributes we retrieved from the worklist, that we 
    ** should have values for.
    */
    if ( SetNullOrStringValue( A_messageID, MC_ATT_PATIENT_ID,
                               A_patient->patientID,
                               "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( A_messageID, MC_ATT_PATIENTS_NAME,
                               A_patient->patientName,
                               "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( A_messageID, MC_ATT_PATIENTS_SEX,
                               A_patient->patientSex,
                               "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( itemID, MC_ATT_REQUESTED_PROCEDURE_ID,
                               A_patient->procedureID,
                               "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( itemID, MC_ATT_REQUESTED_PROCEDURE_DESCRIPTION,
                               A_patient->procedureDesc,
                               "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( itemID, MC_ATT_SCHEDULED_PROCEDURE_STEP_ID,
                               A_patient->stepID,
                               "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( itemID,
                               MC_ATT_SCHEDULED_PROCEDURE_STEP_DESCRIPTION,
                               A_patient->stepID,
                               "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( itemID, MC_ATT_ACCESSION_NUMBER,
                               A_patient->accession,
                               "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( A_messageID, MC_ATT_PATIENTS_BIRTH_DATE,
                               A_patient->patientBirthDate,
                               "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( A_messageID, MC_ATT_PATIENTS_SEX,
                               A_patient->patientSex,
                               "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }
    /*
    ** Now set the attributes that we do not have from the worklist, but
    ** are type 2, so we decide to send these due to the IHE requirements.
    **  Use the SetNullOrStringValue function
    ** with "" as the sent string to force these to NULL.  If you have a
    ** value for these, simply replace the "" with the value string.
    */
    if ( SetNullOrStringValue( A_messageID, MC_ATT_PERFORMED_STATION_NAME,
                               "", "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }
 
    if ( SetNullOrStringValue( A_messageID, MC_ATT_PERFORMED_LOCATION,
                               "", "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( A_messageID,
                               MC_ATT_PERFORMED_PROCEDURE_STEP_DESCRIPTION,
                               "", "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( A_messageID,
                               MC_ATT_PERFORMED_PROCEDURE_TYPE_DESCRIPTION,
                               "", "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( A_messageID, MC_ATT_STUDY_ID,
                               "", "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }


    /*
    ** Now, set several tags that will be set in the N-SET message, but we 
    ** don't know yet.
    */
    if ( SetNullOrStringValue( A_messageID,
                               MC_ATT_PERFORMED_PROCEDURE_STEP_END_DATE,
                               "", "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }

    if ( SetNullOrStringValue( A_messageID,
                               MC_ATT_PERFORMED_PROCEDURE_STEP_END_TIME,
                               "", "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }

    
    
    /*
    ** Now, set several sequence attributes to NULL that are required,
    ** but we don't support.  Note that we're still using the
    ** SetNullOrStringValue function.  We probably should have written a
    ** seperate function that just does sequences.
    */
    if ( SetNullOrStringValue( itemID, MC_ATT_REFERENCED_STUDY_SEQUENCE,
                               "", "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }
    
    if ( SetNullOrStringValue( itemID,
                               MC_ATT_SCHEDULED_ACTION_ITEM_CODE_SEQUENCE,
                               "", "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }
    
    if ( SetNullOrStringValue( A_messageID, MC_ATT_REFERENCED_PATIENT_SEQUENCE,
                               "", "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }
    
    if ( SetNullOrStringValue( A_messageID, MC_ATT_PROCEDURE_CODE_SEQUENCE,
                               "", "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }
    
    if ( SetNullOrStringValue( A_messageID,
                               MC_ATT_PERFORMED_ACTION_ITEM_SEQUENCE,
                               "", "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }
    
    if ( SetNullOrStringValue( A_messageID, MC_ATT_PERFORMED_SERIES_SEQUENCE,
                               "", "SendNCREATERQ" ) != SUCCESS )
    {
        return ( FAILURE );
    }

    return( SUCCESS );
}


/*****************************************************************************
**
** NAME
**    HandleStatProc
**
** SYNOPSIS
**    static int HandleStatProc
**
** ARGUMENTS
**    int A_applicationID - the application identifier
**
** DESCRIPTION
**    Generate a performed procedure instance when there is no worklist
**    management entry available.
**
** RETURNS
**    SUCCESS or FAILURE
**
** SEE ALSO
**    
*****************************************************************************/
static int
HandleStatProc(
                 int A_applicationID
              )
{
    WORK_PATIENT_REC patient;
    char             entered[INPUT_LEN]="";
    int              i;

    printf( "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n" );
    printf( " ** HANDLE STAT PROCEDURE **\n\n" );
    printf( "In order to process a STAT procedure, a\n" );
    printf( "procedure ID will need to be entered.\n\n" );
    printf( "Please enter a Procedure ID for this patient:\n" );
    printf( "==>" );

    fgets( entered, sizeof( entered ), stdin );

    /*
    ** Strip off trailing non-ascii characters, ie c/r if it is there
    */
    for(i=(int)strlen(entered);i>=0;i--)
    {
#ifdef VXWORKS
        if ( (iscntrl)(entered[i]))
#else
        if (iscntrl(entered[i]))
#endif
            entered[i] = '\0'; 
        else
            break;
    }
    strcpy(  patient.procedureID, entered ); /* Set the procedure ID     */

    printf( "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n" );
    printf( " ** HANDLE STAT PROCEDURE **\n\n" );
    printf( "In order to process a STAT procedure, a\n" );
    printf( "modality will need to be entered.\n\n" );
    printf( "Please enter a modality for this patient:\n" );
    printf( "==>" );

    fgets( entered, sizeof( entered ), stdin );

    /*
    ** Strip off trailing non-ascii characters, ie c/r if it is there
    */
    for(i=(int)strlen(entered);i>=0;i--)
    {
#ifdef VXWORKS
        if ((iscntrl)(entered[i]))
#else
        if (iscntrl(entered[i]))
#endif
            entered[i] = '\0'; 
        else
            break;
    }
    strcpy(  patient.modality, entered ); /* We pretend we are a CT scanner */

    strcpy(  patient.startDate, "" );
    strcpy(  patient.startTime, "" );
    strcpy(  patient.stepID, "" );
    strcpy(  patient.stepDesc, "" );
    strcpy(  patient.physicianName, "" );
    strcpy(  patient.procedureDesc, "" );
    strcpy(  patient.studyInstance, Create_Inst_UID() ); /* create a UID */
    strcpy(  patient.accession, "" );
    strcpy(  patient.patientName, "" );
    strcpy(  patient.patientID, "" );
    strcpy(  patient.patientBirthDate, "" );
    strcpy(  patient.patientSex, "" );

    PerformMPPS( &patient, A_applicationID );
    
    return SUCCESS;
}


/*****************************************************************************
**
** NAME
**    SendNSETRQ
**
** SYNOPSIS
**    static int SendNSETRQ
**
** ARGUMENTS
**    int   A_applicationID - the application ID
**    char *A_affectedSOP   - the MPPS to which we are to act upon
**
** DESCRIPTION
**    This function will send an N_SET request message to the MPPS SCP.
**    The N_SET message will contain a performed series and the associated
**    images.  The status of the performed procedure step isn't updated.
**    To complete the MPPS instance, another function will be used.
**
** RETURNS
**    SUCCESS or FAILURE
**
** SEE ALSO
**    
*****************************************************************************/
static int
SendNSETRQ(
              int   A_applicationID,
              char *A_affectedSOP
          )
{
    MC_STATUS  status;
    int        associationID;
    int        messageID;
    int        rspMsgID;
    char       *rspSrvName;
    MC_COMMAND rspCommand;
    char       mppsStatus[ CS_LEN ];

    printf( "Attempting to N_SET a MPPS with a requested "
            "SOP instance UID of:\n" );
    printf( "%s\n", A_affectedSOP );
    /*
    ** This function simulates the actual imaging process of the sample
    ** modality that this program simulates.  In this case, we attempt to
    ** open the N_SET request message, and let another function take care of
    ** actually building the message, itself.
    */
    status = MC_Open_Message( &messageID, "PERFORMED_PROCEDURE_STEP",
                              N_SET_RQ );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNSETRQ", __LINE__, status,
                  "Unable to open a N_SET request message\n." );
        return ( FAILURE );
    }

    /*
    ** Then, we attempt to create the actual N_SET request message.
    */
    if ( SetNSETRQ( messageID, A_affectedSOP ) != SUCCESS )
    {
        /*
        ** No need for an error message, since SetNSETRQ will log one for us
        ** as to why it failed.
        */
        status = MC_Free_Message( &messageID );
        if ( status != MC_NORMAL_COMPLETION )
        {
            LogError( ERROR_LVL_1, "SendNCREATERQ", __LINE__, status,
                      "Unable to free the N_SET message.  Fatal error,\n"
                      "this server will exit.\n" );
        }
        return( FAILURE );
    }

    /*
    ** After we've successfully create the N_SET request message, we can
    ** attempt to open the association with the MPPS SCP.
    */
    if ( OpenAssociation( A_applicationID, &associationID ) != SUCCESS )
    {
        LogError( ERROR_LVL_1, "SendNSETRQ", __LINE__, status,
                  "Unable to create an association with %s\n",
                  G_destAppTitle );
        MC_Free_Message( &messageID );
        return( FAILURE );
    }

    /*
     * Validate and list the message
     */
    ValMessage      (messageID, "N-SET-RQ");
    WriteToListFile (messageID, "nsetrq.msg");

    
    /*
    ** Once the association is open, we attempt to send the request message.
    */
    status = MC_Send_Request_Message( associationID, messageID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNSETRQ", __LINE__, status,
                  "Unable to send the request message to the SCP.\n" );
        MC_Abort_Association( &associationID );
        MC_Free_Message ( &messageID );
        return ( FAILURE );
    }

    /*
    ** Free up the original request message
    */
    status = MC_Free_Message( &messageID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNSETRQ", __LINE__, status,
                  "Unable to free the N_SET message.  Fatal error,\n"
                  "this server will exit.\n" );
        MC_Abort_Association( &associationID );
        exit( FAILURE );
    }
 

    /*
    ** We can examine the N_SET response message, verifying that the status
    ** comes back as expected.
    */
    status = MC_Read_Message( associationID, G_timeout,
                              &rspMsgID, &rspSrvName, &rspCommand );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNSETRQ", __LINE__, status,
                  "Unable to read the response message from the SCP.\n" );
        MC_Abort_Association( &associationID );
        return ( FAILURE );
    }
   
    /*
     * Validate and list the message
     */
    ValMessage      (rspMsgID, "N-SET-RSP");
    WriteToListFile (rspMsgID, "nsetrsp.msg");


    /*
    ** Although our SCP sends the status in the response, there is no 
    ** guarentee it will be there
    */
    status = MC_Get_Value_To_String( rspMsgID,
                                     MC_ATT_PERFORMED_PROCEDURE_STEP_STATUS,
                                     CS_LEN,
                                     mppsStatus );
    if ( status == MC_NORMAL_COMPLETION )
    {
        printf( "Received performed procedure step status:  %s\n"
                "in N_SET response message.\n", mppsStatus );
    }


    /*
    ** Now that we are done, we'll close the association that
    ** we've formed with the server.
    */
    status = MC_Close_Association( &associationID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_2, "SendNSETRQ", __LINE__, status,
                  "Unable to close the association with "
                  "the MPPS SCP.\n" );
    }

    /*
    ** And we do some cleanup.
    */
    status = MC_Free_Message( &rspMsgID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNSETRQ", __LINE__, status,
                  "Unable to free the N_SET message.  Fatal error,\n"
                  "this server will exit.\n" );
        return( FAILURE );
    }

    return( SUCCESS );
}


/*****************************************************************************
**
** NAME
**    SetNSETRQ
**
** SYNOPSIS
**    static int SetNSETRQ
**
** ARGUMENTS
**    int   A_messageID   - the N_SET response message
**    char *A_affectedSOP - the affected sop instance to set
**
** DESCRIPTION
**    This function will set all of the necessary attributes for an N_SET
**    response message.
**
** RETURNS
**
** SEE ALSO
**
*****************************************************************************/
static int
SetNSETRQ(
             int   A_messageID,
             char *A_affectedSOP
          )
{
    MC_STATUS status;
    int       perfSeriesID;
    int       refImageID;

    /*
    ** Now we need some items, that are part of the main message.
    */
    status = MC_Open_Item( &perfSeriesID,
                           "PERFORMED_SERIES" );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SetNSETRQ", __LINE__, status,
                  "Failed to open the performed series sequence "
                  "item.\n" );
        return ( FAILURE );
    }
    /*
    ** And place reference to the sequence item in its respective place.
    */
    status = MC_Set_Value_From_Int( A_messageID, 
                                    MC_ATT_PERFORMED_SERIES_SEQUENCE,
                                    perfSeriesID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        /*
        ** Since we can't place the item into the message, we need to free
        ** the item before we return with an error.
        */
        MC_Free_Item( &perfSeriesID );
        LogError( ERROR_LVL_1, "SetNSETRQ", __LINE__, status,
                  "Failed to set the performed series sequence "
                  "item.\n" );
        return ( FAILURE );
    }

    status = MC_Open_Item( &refImageID,
                           "REF_IMAGE" );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SetNSETRQ", __LINE__, status,
                  "Failed to open the referenced image sequence "
                  "item.\n" );
        return ( FAILURE );
    }
    /*
    ** And place reference to the sequence item in its respective place.
    */
    status = MC_Set_Value_From_Int( perfSeriesID,
                                    MC_ATT_REFERENCED_IMAGE_SEQUENCE,
                                    refImageID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        /*
        ** Since we can't place the item into the message, we need to free
        ** the item before we return with an error.
        */
        MC_Free_Item( &refImageID );
        LogError( ERROR_LVL_1, "SetNSETRQ", __LINE__, status,
                  "Failed to set the referenced image sequence "
                  "item.\n" );
        return ( FAILURE );
    }

    /*
    ** OK, we now have the message and the sequences setup, so we now need to
    ** start placing attributes in the appropriate places.
    */
    /*
    ** First, we place the requested SOP instance UID into the request message.
    ** This will tell the SCP exactly which MPPS instance we are asking it to
    ** perform the N_SET upon.
    */
    if ( SetStringValue ( A_messageID,
                          MC_ATT_REQUESTED_SOP_INSTANCE_UID,
                          A_affectedSOP,
                          "SetNSETRQ" ) != SUCCESS )
        return ( FAILURE );

    /*
    ** Since we are adding an image record, we need to add all of the tags
    ** that are needed for the performed series sequence item.
    ** The information contained within these tags is being derived at the
    ** point of the imaging function.  That is, we are creating these values
    ** RIGHT NOW!
    */
    if ( SetStringValue ( perfSeriesID,
                          MC_ATT_PERFORMING_PHYSICIANS_NAME,
                          "DR^PHYSICIAN",
                          "SetNSETRQ" ) != SUCCESS )
        return ( FAILURE );

    if ( SetStringValue ( perfSeriesID,
                          MC_ATT_PROTOCOL_NAME,
                          "PROTOCOL",
                          "SetNSETRQ" ) != SUCCESS )
        return ( FAILURE );

    if ( SetStringValue ( perfSeriesID,
                          MC_ATT_OPERATORS_NAME,
                          "OPERATOR^NAME",
                          "SetNSETRQ" ) != SUCCESS )
        return ( FAILURE );

    if ( SetStringValue ( perfSeriesID,
                          MC_ATT_SERIES_INSTANCE_UID,
                          Create_Inst_UID(),
                          "SetNSETRQ" ) != SUCCESS )
        return ( FAILURE );

    if ( SetStringValue ( perfSeriesID,
                          MC_ATT_SERIES_DESCRIPTION,
                          "TESTSERIES",
                          "SetNSETRQ" ) != SUCCESS )
        return ( FAILURE );

    if ( SetStringValue ( perfSeriesID,
                          MC_ATT_RETRIEVE_AE_TITLE,
                          "RETR_AE_TITLE",
                          "SetNSETRQ" ) != SUCCESS )
        return ( FAILURE );



    /*
    ** Now, we're ready for the referenced SOP class UID.  In real life, this
    ** is the SOP class UID that the modality is attempting to USE.  We will
    ** fill it in with the UID for CT images.  This will need to change in an
    ** actual implementation.
    */
    if ( SetStringValue ( refImageID,
                          MC_ATT_REFERENCED_SOP_CLASS_UID,
                          "1.2.840.10008.5.1.4.1.1.2",
                          "SetNSETRQ" ) != SUCCESS )
        return ( FAILURE );

    if ( SetStringValue ( refImageID,
                          MC_ATT_REFERENCED_SOP_INSTANCE_UID,
                          Create_Inst_UID(),
                          "SetNSETRQ" ) != SUCCESS )
        return ( FAILURE );


    status = MC_Set_Value_To_NULL( perfSeriesID,
                         MC_ATT_REFERENCED_STANDALONE_SOP_INSTANCE_SEQUENCE );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SetNSETRQ", __LINE__, status,
                  "Failed to set the referenced standalone "
                  "SOP instance sequence to NULL.\n" );
        return ( FAILURE );
    }
    return( SUCCESS );
}


/*****************************************************************************
**
** NAME
**    SendNSETRQComplete
**
** SYNOPSIS
**    static int SendNSETRQComplete
**
** ARGUMENTS
**    int               A_applicationID        - the application ID
**    char             *A_requestedSOPinstance - the sop instance to act upon
**    WORK_PATIENT_REC *A_patient              - a patient record 
**
** DESCRIPTION
**    This function updates the status of a modality performed procedure
**    step to reflect the fact that this imaging procedure is completed
**    at this simulated modality.
**
** RETURNS
**    SUCCESS or FAILURE
**
** SEE ALSO
**    
*****************************************************************************/
static int
SendNSETRQComplete(
                      int               A_applicationID,
                      char             *A_requestedSOPinstance,
                      WORK_PATIENT_REC *A_patient
                  )
{
    MC_STATUS status;
    int  associationID;
    int  messageID;
    char date[ 9 ]="";
    char time[ 7 ]="";
    int        rspMsgID;
    char       *rspSrvName;
    MC_COMMAND rspCommand;
    char       mppsStatus[ CS_LEN ];

    /*
    ** In order to send the PERFORMED PROCEDURE STEP N_SET message to the
    ** SCP, we need to open a message.
    */
    status = MC_Open_Message( &messageID, "PERFORMED_PROCEDURE_STEP",
                              N_SET_RQ );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNSETRQComplete", __LINE__, status,
                  "Unable to open a message for PERFORMED_PROCEDURE_STEP "
                  "N_SET operation\n" );
        return ( FAILURE );
    }

    /*
    ** Now, we setup all of the needed pieces to the performed procedure
    ** step N_SET message.
    */

    /* 
    ** A group zero element:  requested SOP instance UID.  This UID will have
    ** been returned from the SCP during the N_CREATE call by this SCU to the
    ** SCP.  It is basically a key, created by the SCP that is used by the
    ** SCP to know which data set that the SCU is referencing during the N_SET.
    */
    if ( SetStringValue ( messageID,
                          MC_ATT_REQUESTED_SOP_INSTANCE_UID,
                          A_requestedSOPinstance,
                          "SendNSETRQComplete" ) != SUCCESS )
    {
        MC_Free_Message(&messageID);
        return ( FAILURE );
    }

    /*
    ** Performed procedure step end date
    ** Both the date and time are obtained at this point because this signifies
    ** the actual end of the procedure step.
    */
    GetCurrentDate( date, time );
    if ( SetStringValue ( messageID,
                          MC_ATT_PERFORMED_PROCEDURE_STEP_END_DATE,
                          date,
                          "SendNSETRQComplete" ) != SUCCESS )
    {
        MC_Free_Message(&messageID);
        return ( FAILURE );
    }

    /*
    ** Performed procedure step end time
    */
    if ( SetStringValue ( messageID,
                          MC_ATT_PERFORMED_PROCEDURE_STEP_END_TIME,
                          time,
                          "SendNSETRQComplete" ) != SUCCESS )
    {
        MC_Free_Message(&messageID);
        return ( FAILURE );
    }


    /* 
    ** And finally, we set the status to completed.
    */
    if ( SetStringValue ( messageID,
                          MC_ATT_PERFORMED_PROCEDURE_STEP_STATUS,
                          "COMPLETED",
                          "SendNSETRQComplete" ) != SUCCESS )
    {
        MC_Free_Message(&messageID);
        return ( FAILURE );
    }


    /*
    ** Then, we need to open an association with a modality performed
    ** procedure step SCP.
    */
    if (OpenAssociation( A_applicationID, &associationID ) != SUCCESS )
    {
        MC_Free_Message( &messageID );
        return( FAILURE );
    }
    
    /*
     * Validate and list the message
     */
    ValMessage      (messageID, "N-SET-RQ Complete");
    WriteToListFile (messageID, "nsetrqcmp.msg");

    /*
    ** And finally, we send the N_SET message to the server.
    */
    status = MC_Send_Request_Message( associationID, messageID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNSETRQComplete", __LINE__, status,
                  "Unable to send the request message to the SCP.\n" );
        MC_Free_Message ( &messageID );
        MC_Abort_Association ( &associationID );
        return ( FAILURE );
    }

    /*
    ** Free up the request message.
    */
    status = MC_Free_Message( &messageID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNSETRQComplete", __LINE__, status,
                  "Unable to free the N_SET message.  Fatal error,\n"
                  "this server will exit.\n" );
        MC_Abort_Association ( &associationID );
        exit( FAILURE );
    }
    /*
    ** We can examine the N_SET response message, verifying that the status
    ** comes back as expected.
    */
    status = MC_Read_Message( associationID, G_timeout,
                              &rspMsgID, &rspSrvName, &rspCommand );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNSETRQComplete", __LINE__, status,
                  "Unable to read the response message from the SCP.\n" );
        MC_Abort_Association( &associationID );
        return ( FAILURE );
    }
    
    status = MC_Get_Value_To_String( rspMsgID,
                                     MC_ATT_PERFORMED_PROCEDURE_STEP_STATUS,
                                     CS_LEN,
                                     mppsStatus );
    if ( status == MC_NORMAL_COMPLETION )
    {
        printf( "Received performed procedure step status:  %s\n"
                "in N_SET response message.\n", mppsStatus );
    }


    /*
    ** Now that we are done, we'll close the association that
    ** we've formed with the server.
    */
    status = MC_Close_Association( &associationID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNSETRQComplete", __LINE__, status,
                  "Unable to close the association with "
                  "the MPPS SCP.\n" );
        MC_Free_Message( &rspMsgID );
        return( FAILURE );
    }

    /*
    ** ...and then do some cleanup.
    */
    status = MC_Free_Message( &rspMsgID );
    if ( status != MC_NORMAL_COMPLETION )
    {
        LogError( ERROR_LVL_1, "SendNSETRQComplete", __LINE__, status,
                  "Unable to free the N_SET message.  Fatal error,\n"
                  "this server will exit.\n" );
    }

    return( SUCCESS );
}


/*****************************************************************************
**
** NAME
**    GetOptions - Get the command line options.
**
** SYNOPSIS
**    static int GetOptions
**
** ARGUMENTS
**    int argc    - the number of arguments
**    char **argv - a pointer to the list of command line arguments
**
** DESCRIPTION
**    GetOptions is the routine that parses the command line and sets the
**    global variables associated with each parameter.
**
** RETURNS
**    SUCCESS or FAILURE
**
** SEE ALSO
**
*****************************************************************************/


static int
GetOptions ( int argc, char **argv )
{
    int          ai, ci;
    int          iError = FALSE;

    if ( argc < 2 )
    {
        /*
        ** They didn't enter the remote app name, so give them a usage, then
        ** return FAILURE.
        */
        printf ( "Usage: %s [options] remote_ae_title\n", argv[0] );
        return( FAILURE );
    }
    for ( ai = 1; ai < argc && strcmp ( argv[ai], "--" ); ai ++ )
    {
        if ( argv[ai][0] == '-' )
        {
            for ( ci = 1; ci < (int)strlen ( argv[ai] ); ci++ )
            {
                switch ( argv[ai][ci] )
                {
                    case 'a':        /* Application Title */
                    case 'A':        /* Application Title */
                        if ( argv[ai][ci+1] != '\0' )
                        {
                            iError = TRUE;
                        }
                        else if ( (ai + 1) == argc )
                        {
                            iError = TRUE;
                        }
                        else
                        {
                            strcpy ( G_appTitle, &(argv[ai+1][0]) );
                            ai++;
                            ci = (int)strlen ( argv[ai] );
                            printf( "'-a' received.  Setting local "
                                    "application title to:  '%s'.\n",
                                    G_appTitle );
                        }
                        break;

                    case 'h':        /* Help */
                    case 'H':        /* Help */
                        printf ( "\n" );
                        printf ( "Modality Worklist Service Class User\n" );
                        printf ( "\n" );
                        printf ( "Usage: %s [options] remote_ae_title\n",
                                 argv[0] );
                        printf ( "\n" );
                        printf ( "remote_ae_title   "
                                 "The remote application entity "
                                 "to connect to\n" );
                        printf ( "Options:\n" );
                        printf ( "   -h                "
                                 "Print this help page\n" );
                        printf ( "   -a <ae_title>     "
                                 "Local application entity\n" );
                        printf ( "   -n <host name>    "
                                 "Remote Host Name\n" );
                        printf ( "   -p <number>       "
                                 "Remote Host Port Number\n" );
                        printf ( "   -t <number>       "
                                 "Timeout value\n" );
                        printf ( "   -z <number>       "
                                 "Cancel threshold\n" );
                        printf ( "\n" );
                        return ( FAILURE );

                    case 'n': /* Remote Host Name */
                    case 'N': /* Remote Host Name */
                        if ( argv[ai][ci+1] != '\0' )
                        {
                            iError = TRUE;
                        }
                        else if ( (ai + 1) == argc )
                        {
                            iError = TRUE;
                        }
                        else
                        {
                            strcpy ( G_remoteHost, &(argv[ai+1][0]) );
                            ai++;
                            ci = (int)strlen ( argv[ai] );
                            printf( "'-n' received.  Setting remote "
                                    "host name to:  '%s'.\n",
                                    G_remoteHost );

                        }
                        break;

                    case 'p': /* Port Number */
                    case 'P': /* Port Number */
                        if ( argv[ai][ci+1] != '\0' )
                        {
                            iError = TRUE;
                        }
                        else if ( (ai + 1) == argc )
                        {
                            iError = TRUE;
                        }
                        else
                        {
                            G_portNumber =  strtoul( &(argv[ai+1][0]),
                                                           NULL, 10 );
                            ai++;
                            ci = (int)strlen ( argv[ai] );
                            printf( "'-p' received.  Setting remote "
                                    "port number to:  '%d'.\n",
                                    G_portNumber );
                        }
                        break;

                    case 't': /* Timeout */
                    case 'T': /* Timeout */
                        if ( argv[ai][ci+1] != '\0' )
                        {
                            iError = TRUE;
                        }
                        else if ( (ai + 1) == argc )
                        {
                            iError = TRUE;
                        }
                        else
                        {
                            G_timeout =  strtoul( &(argv[ai+1][0]),
                                                       NULL, 10 );
                            ai++;
                            ci = (int)strlen ( argv[ai] );
                            printf( "'-t' received.  Setting network "
                                    "timeout to:  '%d'.\n",
                                    G_timeout );
                        }
                        break;

                    default:
                        break;

                } /* switch argv */

                if ( iError == TRUE )
                {
                    printf ( "Usage: %s [options] remote_ae_title\n", argv[0] );
                    printf ( "       %s -h\n", argv[0] );
                    return ( FAILURE );
                }

            } /* for ci */

        }
        else
        {

            /* This must be the remote application title */
            strcpy ( G_destAppTitle, &(argv[ai][0]) );
            ci = (int)strlen ( argv[ai] );
            ai++;

        }

    } /* for ai */

    if ( G_destAppTitle[0] == '\0' )

    {
        /* Application Title is a required parameter */

        printf( "Required parameter 'Remote Application Entity"
                " Title' not specified.\n" );
        printf ( "Usage: %s [options] <remote_ae_title>\n", argv[0] );
        return( FAILURE );
    }

    return( SUCCESS );
}

