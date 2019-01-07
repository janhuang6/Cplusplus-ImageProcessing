/*
 * file:	testSCU.cc
 * purpose:	test SCU program
 *              Copied from Merge work_scu.c and modified
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>

#include "worklistutils.h"

/* Function prototypes */
static int
GetOptions ( int argc, char **argv, char* localAppTitle, int &portNumber,
             char *remoteHost, int &timeout, char *remoteAppTitle,
             char *mergeIniFile);
static void DisplayAndPrompt( char *A_userInput,
                              char *A_value,
                              const QueryPara &qpar,
                              int&  A_threshold);

/*****************************************************************************
**
** NAME
**    main - Worklist utils test program.
**    
** SYNOPSIS
**    testSCU [options] remote_application_name
**
**    This program sends a C_FIND message to the service class provider
**    and waits for the results to be sent back.  The results are sent
**    back in the form of a C_FIND reply.
**
*****************************************************************************/

#ifdef VXWORKS
void  workscu(int argc, char** argv);
void workscu(int argc, char** argv)
#else
int  main(int argc, char** argv);
int main(int argc, char** argv)
#endif
{   WorklistUtils *pwl;

    MC_STATUS     status;                /* The status returned via toolkit  */
    int           applicationID;         /* ID of this app, after registered */
    char          localAppTitle[64];
    int           associationID;         /* ID of the association of this    */
                                         /* app, and the server.             */
    char          userInput[INPUT_LEN] = "";  /* Value that the user enters  */
    char          value[INPUT_LEN]="";   /* Value that the user enters       */

    QueryPara    qpar;                  /* Query parameters                 */
    char          entered[INPUT_LEN]=""; /* Buffer for user entries          */
    LIST          patientList;           /* The list of patients to process  */
    WorkPatientRec patient;            /* Patient record                   */
    int           count;                 /* Counter for the # of records     */

    char destAppTitle[ AE_LEN+1 ];
    int  portNumber;
    char remoteHost[ AE_LEN+1 ];
    char mergeIniFile[256];
    int timeout;           /* Timeout value                    */
    int threshold;         /* Reply message cut-off threshold  */

// The following are default values
    strcpy(destAppTitle, "MERGE_WORK_SCP");
    portNumber = 3320;
    strcpy(remoteHost, "161.88.11.214");
    timeout = 10000;
    threshold = 1000;    // default value

    strcpy(localAppTitle, "PMS_WORK_SCU");

    /*
    ** Obtain and parse any command line arguments.
    */
    if ( GetOptions ( argc, argv, localAppTitle, portNumber,
                      remoteHost, timeout, destAppTitle, mergeIniFile)
         == MERGE_FAILURE )
    {
        /*
        ** We have invalid parameters here.  We can just exit.
        */
        return( MERGE_FAILURE );
    }

    memset((void*)&qpar, 0, sizeof(QueryPara));
    strcpy(qpar.localAeTitle, localAppTitle);

    if(MERGE_FAILURE == PerformMergeInitialization( mergeIniFile, &applicationID, localAppTitle )) {
        printf("Software error: Cannot initialize the Merge Toolkit.\n");
        return( MERGE_FAILURE );
    }

    pwl = new WorklistUtils(destAppTitle,
                            portNumber, remoteHost,
                            timeout, threshold);

    /*
    ** To benefit the user, we default the date of the query to be the
    ** current date.
    */
    GetCurrentDate( qpar.scheduledProcStepStartDate, value );

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
        DisplayAndPrompt( userInput, value, qpar, threshold );
        /*
        ** Based on their input, we will decide how to continue.
        */
        switch( userInput[0] )
        {
            case '1':
                /*
                ** Local Application Entity Title
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
                strcpy( qpar.localAeTitle, value );
                break;
            case '2':
                /*
                ** Should the user attempt to enter an invalid date, we
                ** don't let them do so.  NOTE:  we let the other date
                ** functions parse a date range, such that we just do a
                ** simple sanity check here...
                */
                if ( strlen( value ) < 8 )
                {
                    printf( "A date string must be in the form of:\n"
                            "YYYYMMDD, YYYYMMDD-YYYYMMDD, -YYYYMMDD, or "
                            "YYYYMMDD-\n" );
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
                    GetCurrentDate( qpar.scheduledProcStepStartDate, value );
                }
                else
                    strcpy( qpar.scheduledProcStepStartDate, value );
                break;
            case '3':
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
                strcpy( qpar.modality, value );
                break;
            case '4':
                /*
                ** Performing Physician
                */
                strcpy( qpar.scheduledPerformPhysicianName, value );
                break;
            case '5':
                /*
                ** Accession Number
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
                strcpy( qpar.accessionNumber, value );
                break;
            case '6':
                /*
                ** Requesting Physician
                */
                strcpy( qpar.requestingPhysician, value );
                break;
            case '7':
                /*
                ** Patient Name
                */
                strcpy( qpar.patientName, value );
                break;
            case '8':
                /*
                ** Patient ID
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
                strcpy( qpar.patientID, value );
                break;
            case '9':
                /*
                ** Change the reply message cut-off threshold
                */
                if ( strtoul( value, NULL, 10 ) < 5 )
                {
                    printf( "Cannot set to a value, less than 5.\n" );
                    threshold = 5;
                    printf( "Press <return> to continue.\n" );
                    fgets( entered, sizeof( entered ), stdin );
                }
                else
                {
                    threshold = strtoul( value, NULL, 10 );
                }
                pwl->setThreshold(threshold);
                break;
            case 'Q':
            case 'q':
                /*
                ** We need to create an empty list that will hold our
                ** query results.
                */
                if ( LLCreate( &patientList,
                               sizeof( WorkPatientRec ) ) != MERGE_SUCCESS )
                {
                    printf( "ERROR_LVL_1 in main line %d %d: Unable to initialize the linked list...\n", __LINE__, MC_NORMAL_COMPLETION);
                    /*
                    ** This is a rather pesky situation.  We cannot continue
                    ** if we are unable to allocate memory for the linked list.
                    ** We will, however attempt to release this application
                    ** from the tooklit. 
                    */
                    MC_Release_Application( &applicationID );
                    return( MERGE_FAILURE );
                }

                /*
                ** Attempt to open an association with the provider.
                */
                try {
                     associationID = pwl->openAssociation( applicationID );
                }
                catch (...) {
                    /*
                    ** Even though we were unable to open the association
                    ** with the server, we will still attempt to release
                    ** this application from the library before exiting.
                    */
                    LLDestroy( &patientList );
                    printf( "Press <return> to continue.\n" );
                    fgets( entered, sizeof( entered ), stdin );
                    return( MERGE_FAILURE );
                }

                /*
                ** At this point, the user has filled in all of their
                ** message parameters, and then decided to query.
                ** Therefore, we need to construct and send their message
                ** off to the server.
                */
                try {
                    pwl->sendWorklistQuery( associationID, qpar );
                }
                catch (...) {
                    /*
                    ** Before exiting, we will attempt to close the association
                    ** that was successfully formed with the server.
                    */
                    status = MC_Close_Association( &associationID );
                    if ( status != MC_NORMAL_COMPLETION )
                        {
                            printf ( "ERROR_LVL_1 in main: line %d status %d. Unable to close the association with MERGE_WORK_SCP.\n", __LINE__, status );
                            return ( MERGE_FAILURE );
                        }

                    printf("SendWorklistQuery failed.\n");
                    LLDestroy( &patientList );
                    MC_Abort_Association( &associationID );
                    MC_Release_Application( &applicationID );
                    return( MERGE_FAILURE );
                }

                /*
                ** We've now sent out a CFIND message to a service class
                ** provider.  We will now wait for a reply from this server,
                ** and then "process" the reply.
                */
                try {
                    pwl->processWorklistReplyMsg(  associationID, 
													&patientList );
                }
                catch (...) {
                    /*
                    ** Before exiting, we will attempt to close the association
                    ** that was successfully formed with the server.
                    */
                    status = MC_Close_Association( &associationID );
                    if ( status != MC_NORMAL_COMPLETION )
                        {
                            printf ( "ERROR_LVL_1 in main: line %d status %d. Unable to close the association with MERGE_WORK_SCP.\n", __LINE__, status );
                            return ( MERGE_FAILURE );
                        }

                    printf("ProcessWorklistReplyMsg failed.\n");
                    LLDestroy( &patientList );
                    MC_Abort_Association( &associationID );
                    MC_Release_Application( &applicationID );
                    return( MERGE_FAILURE );
                }

                /*
                ** Now that we are done, we'll close the association that
                ** we've formed with the server.
                */
                status = MC_Close_Association( &associationID );
                if ( status != MC_NORMAL_COMPLETION )
                {
                    printf ( "ERROR_LVL_1 in main: line %d status %d. Unable to close the association with MERGE_WORK_SCP.\n", __LINE__, status );
                    LLDestroy( &patientList );
                    return ( MERGE_FAILURE );
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
                else
                {
                    /*
                    ** Print the list of patients here.
                    */
                    printf( "      %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s\n", "PatientName", "PatientID", "PatientDOB", "Height", "Weight", "Gender", "PatientState", "PregnancyStatus",  "MedicalAlerts", "Allergies", "SpecialNeeds", "LocalAETitle", "StartDate", "StartTime", "Modality", "PerformingPhysician", "StepDesc", "StationName", "StepID", "schdProtCodeMeaning", "ProcID", "ProcDesc", "ReqProcCodeMeaning", "StudyInstUID", "Accession#", "RequestingPhysician", "ReferringPhysician", "otherPatientID", "intendedRecipients", "imageServiceRequestComments", "requestedProcComments", "seriesInstanceUID", "reason4RequestProc");
                    printf( "      %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s\n", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------" );

                     /*
                     ** Rewind the list of patients so that we can display the
                     ** lists contents.
                     */
                     LLRewind( &patientList );

                     /*
                     ** Clear the counter.
                     */
                     count = 0;

                     /*
                     ** Now, walk the list, getting each record from it.
                     */
                     while( LLPopNode( &patientList, &patient ) != NULL )
                     {	
                         count++;
                         printf( "[%2d]  %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s ", count, patient.patientsName, patient.patientID, patient.patientsDOB, patient.patientsHeight, patient.patientsWeight, patient.patientsGender, patient.patientState, patient.pregnancyStatus, patient.medicalAlerts, patient.contrastAllergies, patient.specialNeeds, patient.scheduledStationLocalAETitle, patient.scheduledProcedureStepStartDate);
		         printf("%16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s %16.16s", patient.scheduledProcedureStepStartTime, patient.modality, patient.scheduledPerformingPhysiciansName, patient.scheduledProcedureStepDescription, patient.scheduledStationName, patient.scheduledProcedureStepID, patient.scheduledProtocolCodeMeaning, patient.requestedProcedureID, patient.requestedProcedureDescription, patient.requestedProcedureCodeMeaning, patient.studyInstanceUID, patient.accessionNumber, patient.requestingPhysician, patient.referringPhysician );
		         printf("%16.16s %16.16s %16.16s %16.16s %16.16s %16.16s\n", patient.otherPatientID, patient.intendedRecipients, patient.imageServiceRequestComments, patient.requestedProcedureComments, patient.seriesInstanceUID, patient.reasonForRequestedProcedure );
                     }
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

    delete pwl;

    return ( MERGE_SUCCESS );

} /* End of main */



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
**    MERGE_SUCCESS or MERGE_FAILURE
**
** SEE ALSO
**
*****************************************************************************/


static int
GetOptions ( int argc, char **argv, char* localAppTitle, int &portNumber,
             char *remoteHost, int &timeout, char *remoteAppTitle,
             char *mergeIniFile)
{
    int     ai, ci;
    bool     iError = false;

    if ( argc < 2 )
    {
        /*
        ** They didn't enter the remote app name, so give them a usage, then
        ** return MERGE_FAILURE.
        */
        printf ( "Usage: %s [options] remote_ae_title\n", argv[0] );
        return( MERGE_FAILURE );
    }
    for ( ai = 1; ai < argc && strcmp ( argv[ai], "--" ); ai ++ )
    {
        if ( argv[ai][0] == '-' )
        {
            for ( ci = 1; ci < (int)strlen ( argv[ai] ); ci++ )
            {
                switch ( argv[ai][ci] )
                {
                    case 'a':        /* Local Application Title */
                    case 'A':        /* Local Application Title */
                        if ( argv[ai][ci+1] != '\0' )
                        {
                            iError = true;
                        }
                        else if ( (ai + 1) == argc )
                        {
                            iError = true;
                        }
                        else
                        {
                            strcpy ( localAppTitle, &(argv[ai+1][0]) );
                            ai++;
                            ci = strlen ( argv[ai] );
                            printf( "'-a' received.  Setting local "
                                    "application title to:  '%s'.\n",
                                    localAppTitle );
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
                        printf ( "   -m                 "
                                 "Merge INI file with full path\n" );
                        printf ( "   -h                "
                                 "Print this help page\n" );
                        printf ( "   -a <local_ae_title>     "
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
                        return ( MERGE_FAILURE );

                    case 'm': /* Merge INI File with Full Path */
                    case 'M': /* Merge INI File with Full Path */
                        if ( argv[ai][ci+1] != '\0' )
                        {
                            iError = true;
                        }
                        else if ( (ai + 1) == argc )
                        {
                            iError = true;
                        }
                        else
                        {
                            strcpy ( mergeIniFile, &(argv[ai+1][0]) );
                            ai++;
                            ci = strlen ( argv[ai] );
                            printf( "'-m' received.  Setting merge.ini "
                                    "file to:  '%s'.\n",
                                    mergeIniFile );

                        }
                        break;

                    case 'n': /* Remote Host Name */
                    case 'N': /* Remote Host Name */
                        if ( argv[ai][ci+1] != '\0' )
                        {
                            iError = true;
                        }
                        else if ( (ai + 1) == argc )
                        {
                            iError = true;
                        }
                        else
                        {
                            strcpy ( remoteHost, &(argv[ai+1][0]) );
                            ai++;
                            ci = strlen ( argv[ai] );
                            printf( "'-n' received.  Setting remote "
                                    "host name to:  '%s'.\n",
                                    remoteHost );
                        }
                        break;

                    case 'p': /* Port Number */
                    case 'P': /* Port Number */
                        if ( argv[ai][ci+1] != '\0' )
                        {
                            iError = true;
                        }
                        else if ( (ai + 1) == argc )
                        {
                            iError = true;
                        }
                        else
                        {
                            portNumber =  strtoul( &(argv[ai+1][0]),
                                                           NULL, 10 );
                            ai++;
                            ci = strlen ( argv[ai] );
                            printf( "'-p' received.  Setting remote "
                                    "port number to:  '%d'.\n",
                                    portNumber );
                        }
                        break;

                    case 't': /* Timeout */
                    case 'T': /* Timeout */
                        if ( argv[ai][ci+1] != '\0' )
                        {
                            iError = true;
                        }
                        else if ( (ai + 1) == argc )
                        {
                            iError = true;
                        }
                        else
                        {
                            timeout =  strtoul( &(argv[ai+1][0]),
                                                       NULL, 10 );
                            ai++;
                            ci = strlen ( argv[ai] );
                            printf( "'-t' received.  Setting network "
                                    "timeout to:  '%d'.\n",
                                    timeout );
                        }
                        break;

                    default:
                        break;

                } /* switch argv */

                if ( iError == true )
                {
                    printf ( "Usage: %s [options] remote_ae_title\n", argv[0] );
                    printf ( "       %s -h\n", argv[0] );
                    return ( MERGE_FAILURE );
                }

            } /* for ci */

        }
        else
        {
            /* This must be the remote application title */
            strcpy ( remoteAppTitle, &(argv[ai][0]) );
            ci = strlen ( argv[ai] );
            ai++;
        }

    } /* for ai */

    if ( remoteAppTitle[0] == '\0' )

    {
        /* Remote Application Title is a required parameter */

        printf( "Required parameter 'Remote Application Entity"
                " Title' not specified.\n" );
        printf ( "Usage: %s [options] <remote_ae_title>\n", argv[0] );
        return( MERGE_FAILURE );
    }

    return( MERGE_SUCCESS );
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
                  const QueryPara &qpar,
                  int&  A_threshold
                )
{
    char    entered[INPUT_LEN]="";
    int     i;

    printf( "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n" );
    printf( " ** MODALITY WORKLIST SEARCH **\n\n" );
    printf( "Enter the number of the value to modify,\n" );
    printf( "followed by the value that you wish to set.\n" );
    printf( " e.g.:  1 CT\n" );
    printf( "[1] Local AE Title             [%s]\n", qpar.localAeTitle );
    printf( "[2] Procedure Start Date       [%s]\n", qpar.scheduledProcStepStartDate );
    printf( "[3] Modality                   [%s]\n", qpar.modality );
    printf( "[4] Performing Physician       [%s]\n", qpar.scheduledPerformPhysicianName );
    printf( "[5] Accession Number           [%s]\n", qpar.accessionNumber );
    printf( "[6] Requesting Physician       [%s]\n", qpar.requestingPhysician );
    printf( "[7] Patient Name               [%s]\n", qpar.patientName );
    printf( "[8] Patient ID                 [%s]\n", qpar.patientID );
    printf( "[9] Reply message cut-off threshold  [%d]\n", A_threshold );
    printf( "[Q] Perform a query\n" );
    printf( "[X] Exit\n\n" );
    printf( "==>" );
    fflush( stdout );

    fgets( entered, sizeof( entered ), stdin );

    /*
    ** Strip off trailing non-ascii characters, ie c/r if it is there
    */
    for(i=strlen(entered);i>=0;i--)
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
