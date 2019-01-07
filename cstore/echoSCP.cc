/*************************************************************************
 *
 *    $Workfile: echoSCP.cc $
 *
 *    $Revision: 1.6 $
 *
 *        $Date: 2010/03/08 20:22:35 $
 *
 *       Author: Modified by jhuang from Merge program
 *
 *  Description: This is a C-ECHO service class provider application
 *               for the Verification Service Class.
 *
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#include "mcstatus.h"
#include "mergecom.h"
#include "mc3msg.h"
#include "echoSCP.h"
#include "acquire/mesg.h"
#include "acquire/threadmutex.h"

#ifdef linux

static pid_t   Schild_PID[MAX_CHILD_SERVERS] = {0};
static int   server_quota = MAX_CHILD_SERVERS;  /* Max children */   

ThreadMutex g_lock_echo;  /* Serialize access to the critical section.*/

/****************************************************************************
 *
 *  Function    :   echoSCP
 *  Description :   Loops waiting for associations
 *
 ****************************************************************************/
void* echoSCP( void* echo_args )
{
	MutexGuard guard (g_lock_echo);

	ECHO_ARGS       *pEchoArgs;
	MC_STATUS       mcStatus;
	MC_SOCKET       mcSocket;

	static int      quota_message = 1;
	int             child;
	pid_t           childs_pid;
	BOOLEAN         childReject;

	pEchoArgs = (struct ECHO_ARGS*)echo_args;

    /*
      *  Handle change in states of our children
      */
    signal(SIGCHLD, (void (*)(int))reaper);

    /*
     *  Parent will handle shutdown
     */
    signal(SIGINT, (void (*)(int))sigint_routine);


    /*
     *  Handle situation of a process continuing to send data on a closed
     *  connection.
     */
    signal(SIGPIPE, (void (*)(int))shandler);


    /*
     *  Go back to the proper ui so we don't mess with things we don't own.
     */
    setuid(getuid());

    /*
     * Call MC_Wait_For_Connection and then fork a child process
     */
    for (;;)
    {
		if (pEchoArgs->EchoIncomingPort == -1)
			mcStatus = MC_Wait_For_Connection(  0, &mcSocket);
		else
			mcStatus = MC_Wait_For_Connection_On_Port( 0,
												pEchoArgs->EchoIncomingPort,
						                        &mcSocket);

		if (mcStatus == MC_TIMEOUT)
        {
            sleep(5);
            continue;
        }
        else if (mcStatus == MC_UNKNOWN_HOST_CONNECTED)
        {
            ::Message(MWARNING, toEndUser | toService | MLoverall, "\tEchoSCP received connection from an unknown host, association rejected");
			sleep(5);
            continue;
        }
        else if (mcStatus == MC_NEGOTIATION_ABORTED)
        {
            ::Message(MWARNING, toEndUser | toService | MLoverall, "\tEchoSCP received association aborted during negotiation");
			sleep(5);
            continue;
        }
        else if (mcStatus != MC_NORMAL_COMPLETION)
        {
			PrintError("EchoSCP: MC_Wait_For_Association returned error", mcStatus);
			sleep(5);
            continue;
        }
        
		/* 
		 * Process the association request.
	     */
        /*
         * Create a child process to handle the association
         */
        for (child = 0; child < server_quota; child++)
        {
            if (!Schild_PID[child])
                break;     /* We found an available slot */
        }
      
        if (child >= server_quota)
        {
            childReject = TRUE;
            if (quota_message)
            {
                quota_message = 0;
                ::Message(MWARNING, toEndUser | toService | MLoverall, "Server Quota reached at least once");
            }
        }
        else
            childReject = FALSE;
      
        /*
         *  Now fork a child process to handle the association
         */
        childs_pid = fork();
      
        switch (childs_pid)
        {
            case    0:      /* child */
               /*
                *  Children will ignore interrupts
                */
               signal(SIGINT, SIG_IGN);
               signal(SIGCHLD, SIG_DFL);
          
               Handle_EchoAssociation(mcSocket, pEchoArgs->ApplicationID, childReject);
               _exit( 0); // successful return
               /* Children go away when they are done */
     
            case    -1:     /* error */
               ::Message(MWARNING, toEndUser | toService | MLoverall, "Unable to fork child process");
               Handle_EchoAssociation(mcSocket, pEchoArgs->ApplicationID, childReject);
               break;
     
            default :       /* parent */
               Schild_PID[child] = childs_pid;
               break;
        }
      
        /*
         *  Release parent's association resources.  This releases socket
         *  related information and memory that is now being used by the 
         *  child process.
         */
        MC_Release_Parent_Connection(mcSocket);
    }

	guard.release();
	pthread_exit( (void *) &THREAD_NORMAL_EXIT );
}   /* END echoSCP() */


/****************************************************************************
 *
 *  Function    :    Handle_EchoAssociation
 *
 *  Parameters  :    applicationID - the application ID
 *
 *  Returns     :    nothing
 *
 *  Description :    Processes a received association requests
 *
 ****************************************************************************/
static void Handle_EchoAssociation(MC_SOCKET mcSocket, int applicationID, BOOLEAN childReject)
{
    MC_STATUS   mcStatus;
    MC_COMMAND  command;
    int         msgID;
    int         rspMsgID;
    char*       serviceName;
    AssocInfo   asscInfo;
    ServiceInfo servInfo;
    int         nrequests = 0;
    int         calledApplicationID = -1;
    int         associationID = -1;
    BOOLEAN     echo_found;
    
    mcStatus = MC_Process_Association_Request(mcSocket,
                                              "Verification_Service_List",
                                              &calledApplicationID,
                                              &associationID);

    if (mcStatus == MC_ASSOCIATION_CLOSED)
    {
        ::Message(MWARNING, toEndUser | toService | MLoverall, "An echo association was automatically processed.");

        return;
    }
    else if (mcStatus == MC_UNKNOWN_HOST_CONNECTED)
    {
        ::Message(MWARNING, toEndUser | toService | MLoverall, "Unknown host connected, association rejected.");
        return;
    }
    else if (mcStatus == MC_TIMEOUT)
    {
	    PrintError("Error on MC_Process_Association_Request: Time out.", mcStatus);
        return;
    }
    else if (mcStatus == MC_NEGOTIATION_ABORTED)
    {
        ::Message(MWARNING, toEndUser | toService | MLoverall, "Association aborted during negotiation.");
        return;
    }
    else if (mcStatus == MC_SYSTEM_CALL_INTERRUPTED)
    {
	    PrintError("Error on MC_Process_Association_Request: MC_SYSTEM_CALL_INTERRUPTED.", mcStatus);
        return;
    }
    else if (mcStatus == MC_NULL_POINTER_PARM)
    {
	    PrintError("Error on MC_Process_Association_Request: MC_NULL_POINTER_PARM.", mcStatus);
        return;
    }
    else if (mcStatus == MC_NO_APPLICATIONS_REGISTERED)
    {
	    PrintError("Error on MC_Process_Association_Request: MC_NO_APPLICATIONS_REGISTERED.", mcStatus);
        return;
    }
    else if (mcStatus == MC_INVALID_SERVICE_LIST_NAME)
    {
	    PrintError("Error on MC_Process_Association_Request: MC_INVALID_SERVICE_LIST_NAME.", mcStatus);
        return;
    }
    else if (mcStatus == MC_CONFIG_INFO_MISSING)
    {
	    PrintError("Error on MC_Process_Association_Request: MC_CONFIG_INFO_MISSING.", mcStatus);
        return;
    }
    else if (mcStatus == MC_CONFIG_INFO_ERROR)
    {
	    PrintError("Error on MC_Process_Association_Request: MC_CONFIG_INFO_ERROR.", mcStatus);
        return;
    }
    else if (mcStatus == MC_SYSTEM_ERROR)
    {
	    PrintError("Error on MC_Process_Association_Request: MC_SYSTEM_ERROR.", mcStatus);
        return;
    }
    else if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("Error on MC_Process_Association_Request", mcStatus);
        return;
    }

    if ( childReject == TRUE )
    {
        mcStatus = MC_Reject_Association(associationID, TRANSIENT_LOCAL_LIMIT_EXCEEDED);
        ::Message(MNOTE, toEndUser | toService | MLoverall, "C-ECHO association request was rejected because the maximum limit number of simultaneous associations was exceeded.");
        return;
    }

    if (calledApplicationID != applicationID)
    {
        ::Message(MWARNING, toEndUser | toService | MLoverall, "Unexpected application identifier on MC_Process_Association_Request.");
        return;
    }

	/*
     * Our sample simply records association information and then sends the echo reply
     */
    mcStatus = MC_Get_Association_Info( associationID, &asscInfo); 
    if (mcStatus != MC_NORMAL_COMPLETION)
        PrintError("MC_Get_Association_Info failed", mcStatus);
    else
    {
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Connection from Remote Application:");
        ::Message(MNOTE, toEndUser | toService | MLoverall, "   AE Title:                 %s", asscInfo.RemoteApplicationTitle);
        ::Message(MNOTE, toEndUser | toService | MLoverall, "   Host name:                %s", asscInfo.RemoteHostName);
        ::Message(MNOTE, toEndUser | toService | MLoverall, "   IP Address:               %s", asscInfo.RemoteIPAddress);
        ::Message(MNOTE, toEndUser | toService | MLoverall, "   Implementation Version:   %s", asscInfo.RemoteImplementationVersion);
        ::Message(MNOTE, toEndUser | toService | MLoverall, "   Implementation Class UID: %s", asscInfo.RemoteImplementationClassUID);
        ::Message(MNOTE, toEndUser | toService | MLoverall, "   Requested AE Title:       %s", asscInfo.LocalApplicationTitle);
    }

    ::Message(MNOTE, toEndUser | toService | MLoverall, "Services and transfer syntaxes negotiated:");

    /*
     *  Get the services negotiated and check if STANDARD_ECHO is one of them
     */
    echo_found = FALSE;
    mcStatus = MC_Get_First_Acceptable_Service(associationID, &servInfo);
    while (mcStatus == MC_NORMAL_COMPLETION)
    {
        ::Message(MNOTE, toEndUser | toService | MLoverall, "   %-30s: %s\n",servInfo.ServiceName, 
                         GetSyntaxDescription(servInfo.SyntaxType));
            
        if ( !strcmp(servInfo.ServiceName, "STANDARD_ECHO") )
            echo_found = TRUE;
        mcStatus = MC_Get_Next_Acceptable_Service(associationID,&servInfo);
    }

    /*
     * Catch error, but because it doesn't impact operation, don't do 
     * anything about it.
     */
    if (mcStatus != MC_END_OF_LIST)
        PrintError("Warning: Unable to get service info",mcStatus);
    else if ( !echo_found )
    {
        /* 
            Under certain conditions the library will automatically process
            C_ECHO_RQ messages (without notifying the application) even when
            STANDARD_ECHO is not negotiated.
        */
        ::Message(MNOTE, toEndUser | toService | MLoverall, "STANDARD_ECHO not negotiated!");
    }

    /*
     * Accept the association
     */
    ::Message(MNOTE, toEndUser | toService | MLoverall, "Accepting C-ECHO-RQ association");
    mcStatus = MC_Accept_Association(associationID);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        /*
         * Make sure the association is cleaned up.
         */
        MC_Reject_Association(associationID,
                              TRANSIENT_NO_REASON_GIVEN);
        PrintError("Error on MC_Accept_Association", mcStatus);
        return;
    }

    /*
     *  Echo as long as the SCU continues to send ECHO_RQ messages
     */
    for (;;)
    {
        mcStatus = MC_Read_Message( associationID, 
                                    30, 
                                    &msgID,
                                    &serviceName, 
                                    &command);
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            MC_Free_Message(&msgID);
            if (mcStatus == MC_ASSOCIATION_CLOSED)
            {       
                ::Message(MNOTE, toEndUser | toService | MLoverall, "Association Closed.");
                break;
            }
            else if (mcStatus == MC_NETWORK_SHUT_DOWN
                 ||  mcStatus == MC_ASSOCIATION_ABORTED
                 ||  mcStatus == MC_INVALID_MESSAGE_RECEIVED
                 ||  mcStatus == MC_CONFIG_INFO_ERROR)
            {
                /*
                 * In this case, the association has already been closed
                 * for us.
                 */
                PrintError("Unexpected event", mcStatus);
                break;
            }
            
            PrintError("Error on MC_Read_Message", mcStatus);
            MC_Abort_Association(&associationID);
            break;
        }
     

        /*
         *  Since we only negotiated for the echo service we should only
         *  be getting C_ECHO_RQ messages
         */
        if (command != C_ECHO_RQ)
        {
            ::Message(MNOTE, toEndUser | toService | MLoverall, "Unexpected message command received: %d.", (int)command);
            MC_Abort_Association(&associationID);
            break;
        }

        nrequests++;    /* We track how many echo requests we get */
        
        /*
         *  Acquire a response message object, send it, and free it
         */
        mcStatus = MC_Open_Message (&rspMsgID, serviceName, C_ECHO_RSP);
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            PrintError("MC_Open_Message failed", mcStatus);
            MC_Abort_Association(&associationID);
            break;
        }

        /*
         *  We can free our received message now
         *  after we use "serviceName" string, since that string is
         *  freed by the library when we free the message object.
         */
        mcStatus = MC_Free_Message(&msgID);
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            PrintError("MC_Free_Message for request message error", mcStatus);
            MC_Abort_Association(&associationID);
            break;
        }

        /*
         * We must always send success response (per the service class)
         */
        mcStatus = MC_Send_Response_Message(associationID, C_ECHO_SUCCESS, rspMsgID);
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            PrintError("MC_Send_Response_Message failed", mcStatus);
            MC_Abort_Association(&associationID);
            MC_Free_Message(&rspMsgID);
            break;
        }
      
        /* 
         *  Free the response message
         */
        mcStatus = MC_Free_Message(&rspMsgID);
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            PrintError("MC_Free_Message for response message failed", mcStatus);
            MC_Abort_Association(&associationID);
            break;
        }
    }

    if (nrequests)
        ::Message(MNOTE, toEndUser | toService | MLoverall, "%d C_ECHO requests received", nrequests);
    else
        ::Message(MNOTE, toEndUser | toService | MLoverall, "No C_ECHO requests received");

	return;
    
}  /* Handle_EchoAssociation */


/*
 * The following functions are signal functions used under UNIX.  They are
 * mainly used to clean up when a process is closed and to keep track of
 * how many open child processes handling associations are in use.
 */

/****************************************************************************
 *
 *  Function    :   shandler (UNIX)
 *
 *  Description :   The shandler function is used to handle SIGPIPE
 *                  interrupts which occur when a process continues to
 *                  send data on a connection which has closed.
 *
 ****************************************************************************/
static int shandler (   int                 Asigno,
                        long                Acode,
                        struct sigcontext*  Ascp )
{
    signal(SIGPIPE, (void (*)(int))shandler);
    return -1;
}


/****************************************************************************
 *
 *  Function    :   sigint_routine (UNIX)
 *
 *  Description :   This is our SIGINT routine (The user wants to shut down)
 *
 ****************************************************************************/
static void sigint_routine (
                        int                 Asigno,
                        long                Acode,
                        struct sigcontext*  Ascp )
{
    pid_t    pid;
    int      child;
    int      Return = EXIT_SUCCESS;
    
    for (child = 0; child < server_quota; child++)
    {
        pid = Schild_PID[child];
        if (pid)
        {
            /*
              Kill any lingering child processes.
             */
            (void)kill(pid, SIGKILL);
            Return = EXIT_FAILURE;
            fprintf(stderr, "KILLING CHILD PID (%d)\n", pid);
        }
    }   
    exit(Return);
}

/****************************************************************************
 *
 *  Function    :   reaper (UNIX)
 *
 *  Description :   This is our SIGCHLD routine. (One or more of our children
 *                  has changed state)
 *
 ****************************************************************************/
static void reaper(     int                 Asigno,
                        long                Acode,
                        struct sigcontext*  Ascp )
{
   /* Notice we RESET the signal handler before we exit
      this handler.  If we don't SYSTEM V will lose further
      signals.          */

    pid_t   pid;
    int   child;
    int   Return;
   
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
    {
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Reaper being called for Process = %d", pid);
        for (child = 0; child < server_quota; child++)
        {
            if (pid == Schild_PID[child])
            {
                /*
                   This does NOT kill the process; it only checks if
                   the process exists.
                 */
                Return = kill(pid, 0);
                if (Return < 0 && errno == ESRCH)
                   Schild_PID[child] = 0;
                signal(SIGCHLD, (void (*)(int))reaper);
                break;
            }
        }
      
        if (child >= server_quota)
            fprintf(stderr, "WAITPID RETURNED PID=%d (NOT OUR CHILD)\n", pid);
    }
    signal(SIGCHLD, (void (*)(int))reaper);
}

#endif

/****************************************************************************
 *
 *  Function    :   GetSyntaxDescription
 *
 *  Description :   Return a text description of a DICOM transfer syntax.
 *                  This is used for display purposes.
 *
 ****************************************************************************/
const char* GetSyntaxDescription(TRANSFER_SYNTAX A_syntax)
{
    const char* ptr = NULL;
    
    switch (A_syntax)
    {
    case IMPLICIT_LITTLE_ENDIAN: ptr = "Implicit VR Little Endian"; break;
    case EXPLICIT_LITTLE_ENDIAN: ptr = "Explicit VR Little Endian"; break;
    case EXPLICIT_BIG_ENDIAN:    ptr = "Explicit VR Big Endian"; break;
    case IMPLICIT_BIG_ENDIAN:    ptr = "Implicit VR Big Endian"; break;
    case DEFLATED_EXPLICIT_LITTLE_ENDIAN: ptr = "Deflated Explicit VR Little Endian"; break;
    case RLE:                    ptr = "RLE"; break;
    case JPEG_BASELINE:          ptr = "JPEG Baseline (Process 1)"; break;
    case JPEG_EXTENDED_2_4:      ptr = "JPEG Extended (Process 2 & 4)"; break;
    case JPEG_EXTENDED_3_5:      ptr = "JPEG Extended (Process 3 & 5)"; break;
    case JPEG_SPEC_NON_HIER_6_8: ptr = "JPEG Spectral Selection, Non-Hierarchical (Process 6 & 8)"; break;
    case JPEG_SPEC_NON_HIER_7_9: ptr = "JPEG Spectral Selection, Non-Hierarchical (Process 7 & 9)"; break;
    case JPEG_FULL_PROG_NON_HIER_10_12: ptr = "JPEG Full Progression, Non-Hierarchical (Process 10 & 12)"; break;
    case JPEG_FULL_PROG_NON_HIER_11_13: ptr = "JPEG Full Progression, Non-Hierarchical (Process 11 & 13)"; break;
    case JPEG_LOSSLESS_NON_HIER_14: ptr = "JPEG Lossless, Non-Hierarchical (Process 14)"; break;
    case JPEG_LOSSLESS_NON_HIER_15: ptr = "JPEG Lossless, Non-Hierarchical (Process 15)"; break;
    case JPEG_EXTENDED_HIER_16_18: ptr = "JPEG Extended, Hierarchical (Process 16 & 18)"; break;
    case JPEG_EXTENDED_HIER_17_19: ptr = "JPEG Extended, Hierarchical (Process 17 & 19)"; break;
    case JPEG_SPEC_HIER_20_22:   ptr = "JPEG Spectral Selection Hierarchical (Process 20 & 22)"; break;
    case JPEG_SPEC_HIER_21_23:   ptr = "JPEG Spectral Selection Hierarchical (Process 21 & 23)"; break;
    case JPEG_FULL_PROG_HIER_24_26: ptr = "JPEG Full Progression, Hierarchical (Process 24 & 26)"; break;
    case JPEG_FULL_PROG_HIER_25_27: ptr = "JPEG Full Progression, Hierarchical (Process 25 & 27)"; break;
    case JPEG_LOSSLESS_HIER_28:  ptr = "JPEG Lossless, Hierarchical (Process 28)"; break;
    case JPEG_LOSSLESS_HIER_29:  ptr = "JPEG Lossless, Hierarchical (Process 29)"; break;
    case JPEG_LOSSLESS_HIER_14:  ptr = "JPEG Lossless, Non-Hierarchical, First-Order Prediction"; break;
    case JPEG_2000_LOSSLESS_ONLY:ptr = "JPEG 2000 Lossless Only"; break;
    case JPEG_2000:              ptr = "JPEG 2000"; break;
#ifdef linux
    case JPEG_2000_MC_LOSSLESS_ONLY: ptr = "JPEG 2000 Part 2 Multi-component Lossless Only"; break;
    case JPEG_2000_MC:           ptr = "JPEG 2000 Part 2 Multi-component"; break;
    case JPEG_LS_LOSSLESS:       ptr = "JPEG-LS Lossless"; break;
    case JPEG_LS_LOSSY:          ptr = "JPEG-LS Lossy (Near Lossless)"; break;
    case MPEG2_MPML:             ptr = "MPEG2 Main Profile @ Main Level"; break;
#endif
    case PRIVATE_SYNTAX_1:       ptr = "Private Syntax 1"; break;
    case PRIVATE_SYNTAX_2:       ptr = "Private Syntax 2"; break;
    case INVALID_TRANSFER_SYNTAX:ptr = "Invalid Transfer Syntax"; break;
    }
    return ptr;
}

/****************************************************************************
 *
 *  Function    :   PrintError
 *
 *  Description :   Display a text string on one line and the error message
 *                  for a given error on the next line.
 *
 ****************************************************************************/
void PrintError(const char* A_string, MC_STATUS A_status)
{
    char        prefix[30] = {0};
    /*
     *  Need process ID number for messages
     */
#ifdef linux    
    sprintf(prefix, "PID %d", getpid() );
#endif 
    if (A_status == -1)
    {
        ::Message( MWARNING, toEndUser | toService | MLoverall, 
                  "%s\t%s",prefix,A_string);
    }
    else
    {
        ::Message( MWARNING, toEndUser | toService | MLoverall, 
                  "%s\t%s:", prefix, A_string);
        ::Message( MWARNING, toEndUser | toService | MLoverall, 
                  "%s\t\t%s", prefix, MC_Error_Message(A_status));
    }
}
