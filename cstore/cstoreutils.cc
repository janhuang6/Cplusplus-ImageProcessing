/*
 * file:	cstoreutils.cc
 * purpose:	Implementation of cstore utility library
 * created:	24-Aug-07
 * property of:	Philips Nuclear Medicine
 *
 * revision history:
 *   24-Aug-07	Jiantao Huang		    initial version
 */

#include <stdlib.h>
#include <ctype.h>
#include <iostream>
#include <iomanip> 
#include <fstream> 
#include <stdarg.h>
#include <fstream> 

#include "cstoreutils.h"

const int MAX_LOOP_ITERATIONS = 604800; // number of seconds in a week, boz some StorageCommittment can get back to us days later

ThreadMutex g_lock_store;   /* Serialize access to the critical section.*/
ThreadMutex g_lock_sync_commit, g_lock_async_commit, g_lock_either_commit;  /* Serialize access to the critical section.*/

char LocalSystemCallingAE[AE_LENGTH+2];
int	AsyncCommitIncomingPort;
char DefaultTransferSyntax[32];

/*****************************************************************************
**
** NAME
**    PerformInitialization
**
** SYNOPSIS
**    Does the library initialization, registers this as an application,
**    and sets up some initial program values
**
**    This method should only be called once per
**    application. If called more than once, will
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
		::Message( MWARNING, MLoverall | toDeveloper, "MC_Set_MergeINI failed: line %d status %d.", __LINE__, status);
        return( false );
    }

    /*
    ** This is the initial call to the library.  It must be the first call
    ** to the toolkit that is made after MC_Set_MergeINI call
    */
    status = MC_Library_Initialization( NULL, NULL, NULL );
    if ( status == MC_LIBRARY_ALREADY_INITIALIZED )
    {
		::Message( MWARNING, MLoverall | toDeveloper, "PerformInitialization: line %d status %d. The library is already initialized", __LINE__, status);
        return( true );
    }

    if ( status != MC_NORMAL_COMPLETION )
    {
		::Message( MWARNING, MLoverall | toDeveloper, "PerformInitialization: line %d status %d. Unable to initialize the library", __LINE__, status);
        return( false );
    }

    /*
    ** Once we've successfully initialized the library, then we must
    ** register this application with it.  "p_localAppTitle" is
    ** entered on the command line.
    */
    status = MC_Register_Application( p_applicationID, p_localAppTitle );

    if ( status != MC_NORMAL_COMPLETION )
    {
        ::Message( MWARNING, MLoverall | toDeveloper, "PerformInitialization: line %d status %d. Unable to register this application (calling AE Title = %s) with the library", __LINE__, status,  p_localAppTitle);
        return( false );
    }

    return( true );
}

/*
 * StorageData class.
 */

StorageData::StorageData()
{
	_instanceList=NULL;
}

StorageData::~StorageData()
{
	freeInstanceList();
}

StorageData::StorageData(const StorageData& obj)
			: _instanceList (NULL)
// _filenames will be copy constructed by createLinkedList
{
	createLinkedList(obj._filenames);
}

StorageData& StorageData::operator=(const StorageData& obj)
{
// _filenames will be assigned by createLinkedList
	createLinkedList(obj._filenames);

	return *this;
}

void StorageData::clear()
{
	_filenames.clear();
	freeInstanceList();
}

/****************************************************************************
 *
 *  Function    :   addFileToList
 *
 *  Parameters  :   A_fname    - The name of file to add to the list
 *
 *  Returns     :   true
 *                  false
 *
 *  Description :   Create a node in the instance list for a file to be sent
 *                  on this association.  The node is added to the end of the
 *                  list.
 *
 ****************************************************************************/
bool StorageData::addFileToList(char* A_fname)
{
    InstanceNode*    newNode;
    InstanceNode*    listNode;

    newNode = (InstanceNode*)malloc(sizeof(InstanceNode));
    if (!newNode)
    {
        PrintError("Unable to allocate object to store instance information", MC_NORMAL_COMPLETION);
        return ( false );
    }

    memset( newNode, 0, sizeof(InstanceNode) );

    strncpy(newNode->fname, A_fname, sizeof(newNode->fname));
    newNode->fname[sizeof(newNode->fname)-1] = '\0';
    
    newNode->responseReceived = false;
    newNode->failedResponse = false;
    newNode->imageSent = false;
    newNode->msgID = -1;

    newNode->transferSyntax = IMPLICIT_LITTLE_ENDIAN;
	if(!strncmp(DefaultTransferSyntax, "IMPLICIT_BIG_ENDIAN", sizeof("IMPLICIT_BIG_ENDIAN")))
	{	newNode->transferSyntax = IMPLICIT_BIG_ENDIAN;
		::Message(MNOTE, toEndUser | toService | MLoverall, "Set transferSyntax = IMPLICIT_BIG_ENDIAN");
#ifdef DEBUG_PRINTF
		printf("Set transferSyntax = IMPLICIT_BIG_ENDIAN\n");
#endif
	}
	else if(!strncmp(DefaultTransferSyntax, "EXPLICIT_LITTLE_ENDIAN", sizeof("EXPLICIT_LITTLE_ENDIAN")))
	{	newNode->transferSyntax = EXPLICIT_LITTLE_ENDIAN;
		::Message(MNOTE, toEndUser | toService | MLoverall, "Set transferSyntax = EXPLICIT_LITTLE_ENDIAN");
#ifdef DEBUG_PRINTF
		printf("Set transferSyntax = EXPLICIT_LITTLE_ENDIAN\n");
#endif
	}
	else if(!strncmp(DefaultTransferSyntax, "EXPLICIT_BIG_ENDIAN", sizeof("EXPLICIT_BIG_ENDIAN")))
	{	newNode->transferSyntax = EXPLICIT_BIG_ENDIAN;
		::Message(MNOTE, toEndUser | toService | MLoverall, "Set transferSyntax = EXPLICIT_BIG_ENDIAN");
#ifdef DEBUG_PRINTF
		printf("Set transferSyntax = EXPLICIT_BIG_ENDIAN\n");
#endif
	}
    
	newNode->storageStatus = DICOMStoragePkg::STORAGE_UNKNOWN;
	newNode->commitStatus = DICOMStoragePkg::COMMIT_UNKNOWN;
    newNode->Next = NULL;

    if ( !_instanceList )
    {
        /*
         * Nothing in the list
         */
        _instanceList = newNode;
    }
    else
    {
        /*
         * Add to the tail of the list
         */
        listNode = _instanceList;
        
        while ( listNode->Next )
            listNode = listNode->Next;
      
        listNode->Next = newNode;
    }
    
    return ( true );
}

/****************************************************************************
 *
 *  Function    :   freeInstanceList
 *
 *  Parameters  :   None
 *
 *  Returns     :   nothing
 *
 *  Description :   Free the memory allocated for a list of nodes transferred
 *
 ****************************************************************************/
void StorageData::freeInstanceList()
{
    InstanceNode *node, *next;
    
	next = _instanceList;
    /*
     * Free the instance list
     */
    while (next)
    {
        node = next;
        next = node->Next;
        
        if ( node->msgID != -1 )
            MC_Free_Message(&node->msgID);
        
        free( node );
    }

	_instanceList = NULL;
}


void StorageData::createLinkedList(const list<string>& filelist)
{	list<string>::const_iterator iter;
	char filename[256];

	clear();
	_filenames = filelist;

	/*
     * Create a linked list of all files to be transferred.
     */
    for(iter=_filenames.begin(); iter != _filenames.end(); ++iter)
	{  
	   memset(filename, 0, sizeof(filename));
	   strcpy(filename, iter->c_str());

       if (!addFileToList( filename ))
       {
         ::Message( MWARNING, toEndUser | toService | MLoverall, 
                   "Warning, cannot add filename to File List, image [%s] will not be sent", filename);
       }
	}
}


bool StorageData::isEmpty()
{
	return _instanceList == NULL;
}

STORE_ARGS::~STORE_ARGS()
{
	return; // The thread must preserve the pointer StorageData* as a return value
}

COMMIT_ARGS::COMMIT_ARGS()
			: result (NULL)
{
}

COMMIT_ARGS::COMMIT_ARGS(const COMMIT_ARGS& obj)
			: options (obj.options),
			  appID (obj.appID),
			  result (obj.result)  // We need shadow copy
{
}

COMMIT_ARGS& COMMIT_ARGS::operator=(const COMMIT_ARGS& obj)
{
	options = obj.options;
	appID = obj.appID;
	result = obj.result;	// We need shadow copy

	return *this;
}

COMMIT_ARGS::~COMMIT_ARGS()
{
	return; // The thread must preserve the pointer ResultByStorageTarget* as a return value
}

/****************************************************************************
 *
 *  Function    :   StoreFiles
 *
 *  Parameters  :   store_args includes:
 *                  applicationID - unique MERGE application ID
 *					options		  - storage parameters
 *					pStorageData  - pointer to a list of files to be stored
 *
 *  Returns     :   true
 *                  false
 *
 *  Description :   Main function to store the images.
 *
 ****************************************************************************/
void* StoreFiles(void* store_args)
{
	MutexGuard guard (g_lock_store);

	bool                    tempBool;
    MC_STATUS               mcStatus;
    int                     associationID = -1;
    time_t                  assocStartTime = 0L;
    time_t                  assocEndTime = 0L;
    time_t                  imageStartTime = 0L;
    time_t                  imageEndTime = 0L;
    float                   totalTime = 0L;
    ServiceInfo             servInfo;
    size_t                  totalBytesRead = 0L;
    int                     imagesSent = 0L;
    int                     totalImages = 0L;
    InstanceNode*           node = NULL;
	InstanceNode*           instanceList;
	STORE_ARGS*             storeArgs;

	storeArgs = (STORE_ARGS*)store_args;
	instanceList = storeArgs->storageData->_instanceList;

	totalImages = GetNumNodes( instanceList );
    
    if (totalImages == 0)
    {
        ::Message(MWARNING, toEndUser | toService | MLoverall, "Zero Number of Files to Store!");

		delete storeArgs;
		guard.release();
        pthread_exit( (void *) &THREAD_NORMAL_EXIT );
    }

    if (storeArgs->options.Verbose)
    {
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Opening connection to remote system for DICOM storage:");
        ::Message(MNOTE, toEndUser | toService | MLoverall, "    AE title: %s", storeArgs->options.RemoteAE);
        if (storeArgs->options.RemoteHostname[0])
            ::Message(MNOTE, toEndUser | toService | MLoverall, "    Hostname: %s", storeArgs->options.RemoteHostname);
        else
            ::Message(MNOTE, toEndUser | toService | MLoverall, "    Hostname: Default in mergecom.app");
        
        if (storeArgs->options.RemotePort != -1)
            ::Message(MNOTE, toEndUser | toService | MLoverall, "        Port: %d", storeArgs->options.RemotePort);
        else
            ::Message(MNOTE, toEndUser | toService | MLoverall, "        Port: Default in mergecom.app");
        
        if (storeArgs->options.ServiceList[0])
            ::Message(MNOTE, toEndUser | toService | MLoverall, "Service List: %s", storeArgs->options.ServiceList);
        else
            ::Message(MNOTE, toEndUser | toService | MLoverall, "Service List: Default in mergecom.app");
            
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Number of Files to Store: %d", totalImages);
    }

    assocStartTime = time(NULL);
     
    /*
     *   Open association and override hostname & port parameters if 
     *   they were supplied.
     */
    mcStatus = MC_Open_Association( storeArgs->applicationID, &associationID,
                                    storeArgs->options.RemoteAE,
                                    storeArgs->options.RemotePort != -1 ? &storeArgs->options.RemotePort : 0, 
                                    storeArgs->options.RemoteHostname[0] ? storeArgs->options.RemoteHostname : NULL,
                                    storeArgs->options.ServiceList[0] ? storeArgs->options.ServiceList : NULL );
                                    
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        ::Message(MWARNING, toEndUser | toService | MLoverall, "\t%s", MC_Error_Message(mcStatus));
        ::Message(MWARNING, toEndUser | toService | MLoverall, "Unable to open association with \"%s\":", storeArgs->options.RemoteAE);

		delete storeArgs;
		guard.release();
        pthread_exit( (void *) &THREAD_EXCEPTION );
    }
   
    mcStatus = MC_Get_Association_Info( associationID, &storeArgs->options.asscInfo); 
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Get_Association_Info failed", mcStatus);
    }

    if (storeArgs->options.Verbose)
    {
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Connecting to Remote Application:");
        ::Message(MNOTE, toEndUser | toService | MLoverall, "  Remote AE Title:          %s", storeArgs->options.asscInfo.RemoteApplicationTitle);
        ::Message(MNOTE, toEndUser | toService | MLoverall, "  Local AE Title:           %s", storeArgs->options.asscInfo.LocalApplicationTitle);
        ::Message(MNOTE, toEndUser | toService | MLoverall, "  Host name:                %s", storeArgs->options.asscInfo.RemoteHostName);
        ::Message(MNOTE, toEndUser | toService | MLoverall, "  IP Address:               %s", storeArgs->options.asscInfo.RemoteIPAddress);
#ifdef linux
		::Message(MNOTE, toEndUser | toService | MLoverall, "  Local Max PDU Size:       %ld", storeArgs->options.asscInfo.LocalMaximumPDUSize);
        ::Message(MNOTE, toEndUser | toService | MLoverall, "  Remote Max PDU Size:      %ld", storeArgs->options.asscInfo.RemoteMaximumPDUSize);
        ::Message(MNOTE, toEndUser | toService | MLoverall, "  Max operations invoked:   %d", storeArgs->options.asscInfo.MaxOperationsInvoked);
        ::Message(MNOTE, toEndUser | toService | MLoverall, "  Max operations performed: %d", storeArgs->options.asscInfo.MaxOperationsPerformed);
#endif
        ::Message(MNOTE, toEndUser | toService | MLoverall, "  Implementation Version:   %s", storeArgs->options.asscInfo.RemoteImplementationVersion);
        ::Message(MNOTE, toEndUser | toService | MLoverall, "  Implementation Class UID: %s\n\n", storeArgs->options.asscInfo.RemoteImplementationClassUID);
        
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Services and transfer syntaxes negotiated:");
        
        /*
         * Go through all of the negotiated services. If encapsulated /
         * compressed transfer syntaxes are supported, this code should be
         * expanded to save the services & transfer syntaxes that are 
         * negotiated so that they can be matched with the transfer
         * syntaxes of the images being sent.
         */
		mcStatus = MC_Get_First_Acceptable_Service(associationID,&servInfo);
        while (mcStatus == MC_NORMAL_COMPLETION)
        {
            ::Message(MNOTE, toEndUser | toService | MLoverall, "  %-30s: %s",servInfo.ServiceName, 
                     GetSyntaxDescription(servInfo.SyntaxType));
            
            mcStatus = MC_Get_Next_Acceptable_Service(associationID,&servInfo);
        }
        
        if (mcStatus != MC_END_OF_LIST)
        {
            PrintError("Warning: Unable to get service info",mcStatus);
        }
        
        ::Message(MNOTE, toEndUser | toService | MLoverall, "\n");
    }
    else
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Connected to remote system [%s]\n", storeArgs->options.RemoteAE);
    
    /*
     *   Send all requested images.  Traverse through instanceList to get all 
     *   files to send
     */
    node = instanceList;
    while ( node )
    {
        imageStartTime = time(NULL);

        /*
         * Determine the image format and read the image in.  If the 
         * image is in the part 10 format, convert it into a message.
		 * ReadImage will read the SOPClassUID and SOPInstanceUID from
		 * the image and populate the node.
         */
        tempBool = ReadImage( storeArgs->options, 
                              storeArgs->applicationID, 
                              node);
        if (!tempBool)
        {
            node->imageSent = false;
			::Message(MWARNING, toEndUser | toService | MLoverall, "Cstore will skip this file: UNKNOWN_FORMAT for image [%s]", node->fname);
            node = node->Next;
            continue;
        }
       
        totalBytesRead += node->imageBytes;
         
        /*
         * Send image read in with ReadImage.  
         *
         * Because SendImage may not have actually sent an image even
         * though it has returned success, the calculation of 
         * performance data below may not be correct.
         */
        tempBool = SendImage( storeArgs->options, 
                              associationID, 
                              node);
        if (!tempBool)
        {
            node->imageSent = false;
            ::Message(MWARNING, toEndUser | toService | MLoverall, "Failure in sending file [%s]", node->fname);
            node = node->Next;
            continue;
//            MC_Abort_Association(&associationID);
//            break;
        }
       
        if ( node->imageSent == true )
        {
            /*
             * Save image transfer information in list
             */
            tempBool = UpdateNode( node );
            if (!tempBool)
            {
                ::Message(MWARNING, toEndUser | toService | MLoverall, "Warning, unable to update node with information [%s]", node->fname);
//                MC_Abort_Association(&associationID);
//                break;
            }
            
            imagesSent++;
        }
        else
        {
            node->responseReceived = true;
            node->failedResponse = true;
        }
        
        mcStatus = MC_Free_Message(&node->msgID);
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            PrintError("MC_Free_Message failed for request message", mcStatus);
        }

        /*
         * The following is the core code for handling DICOM asynchronous
         * transfers.  With asynchronous communications, the SCU is allowed
         * to send multiple request messages to the server without 
         * receiving a response message.  The MaxOperationsInvoked is 
         * negotiated over the association, and determines how many request
         * messages the SCU can send before a response must be read.
         *
         * In this code, we first poll to see if a response message is 
         * available.  This means data is readily available to be read over 
         * the connection.  If there is no data to be read & asychronous 
         * operations have been negotiated, and we haven't reached the max
         * number of operations invoked, we can just go ahead and send
         * the next request message.  If not, we go into the loop below
         * waiting for the response message from the server.
         *
         * This code allows network transfer speeds to improve.  Instead of
         * having to wait for a response message, the SCU can immediately 
         * send the next request message so that the connection bandwidth
         * is better utilized.
         */
        tempBool = ReadResponseMessages( storeArgs->options, associationID, 0, &instanceList );
        if (!tempBool)
        {
            ::Message(MWARNING, toEndUser | toService | MLoverall, "Failure in reading response message, aborting association.");
            MC_Abort_Association(&associationID);
            break;
        }
       
#ifdef linux
		/*
         * 0 for MaxOperationsInvoked means unlimited operations.  don't poll if this is the case, just
         * go to the next request to send.
         */
        if ( storeArgs->options.asscInfo.MaxOperationsInvoked > 0 )
            while ( GetNumOutstandingRequests( instanceList ) >= storeArgs->options.asscInfo.MaxOperationsInvoked )
            {
                tempBool = ReadResponseMessages( storeArgs->options, associationID, 10, &instanceList );
                if (!tempBool)
                {
                    ::Message(MWARNING, toEndUser | toService | MLoverall, "Failure in reading response message, aborting association.");
                    MC_Abort_Association(&associationID);
                    break;
                }
            }
#endif

        /*
         *  How long did it take?
         */
        imageEndTime = time(NULL);
        totalTime = (float)(imageEndTime - imageStartTime);
        if ( storeArgs->options.Verbose )
            ::Message(MNOTE, toEndUser | toService | MLoverall, "     Time: %.3f seconds\n", totalTime);
        else
            ::Message(MNOTE, toEndUser | toService | MLoverall, "\tSent %s image (%d of %d), elapsed time: %.3f seconds", node->serviceName, imagesSent, totalImages, totalTime);
        
        /*
         * Traverse through file list
         */
        node = node->Next;
        
    }   /* END for loop for each image */


    /*
     * Wait for any remaining C-STORE-RSP messages.  This will only happen
     * when asynchronous communications are used.
     */
    while ( GetNumOutstandingRequests( instanceList ) > 0 )
    {
        tempBool = ReadResponseMessages( storeArgs->options, associationID, 10, &instanceList );
        if (!tempBool)
        {
            ::Message(MWARNING, toEndUser | toService | MLoverall, "Failure in reading response message, aborting association.");
            MC_Abort_Association(&associationID);
            break;
        }
    }
        
    /*
     * A failure on close has no real recovery.  Abort the association
     * and continue on.
     */
    mcStatus = MC_Close_Association(&associationID);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("Close association failed", mcStatus);
        MC_Abort_Association(&associationID);
    }

    if (storeArgs->options.Verbose)
    {
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Association Closed." );
    }

    /*
     * Calculate the transfer rate.
     */
    assocEndTime = time(NULL);
    totalTime = (float)(assocEndTime - assocStartTime);
    
    /*
     * Check for divide by zero becaue of a quick transfer.
     */
    if (totalTime < 0.000000000001) totalTime = 1.0;
        
    ::Message(MNOTE, toEndUser | toService | MLoverall, "Data Transferred: %luKB", (long)totalBytesRead / 1024 );
    ::Message(MNOTE, toEndUser | toService | MLoverall, "    Time Elapsed: %.3fs", totalTime);
    ::Message(MNOTE, toEndUser | toService | MLoverall, "   Transfer Rate: %.1fKB/s", ((float)totalBytesRead / totalTime) / 1024.0);

	// Do not free the nodelist here. Let the allocator manage it.

	delete storeArgs;
	guard.release();
    pthread_exit( (void *) &THREAD_NORMAL_EXIT );

	return NULL;
}


/****************************************************************************
 *
 *  Function    :   UpdateNode
 *
 *  Parameters  :   A_node     - node to update
 *
 *  Returns     :   true
 *                  false
 *
 *  Description :   Update an image node with info about a file transferred
 *
 ****************************************************************************/
bool UpdateNode(InstanceNode*     A_node)
{
    MC_STATUS        mcStatus;
    
    /*
     * Get DICOM msgID for tracking of responses
     */
    mcStatus = MC_Get_Value_To_UInt(A_node->msgID, 
                    MC_ATT_MESSAGE_ID,
                    &(A_node->dicomMsgID));
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Get_Value_To_UInt for Message ID failed", mcStatus);
        A_node->responseReceived = true;
        return(false);
    }
   
    A_node->responseReceived = false;
    A_node->failedResponse = false;
    A_node->imageSent = true;
    
    return ( true );
}


/****************************************************************************
 *
 *  Function    :   GetNumNodes
 *
 *  Parameters  :   A_list     - Pointer to head of node list to get count for
 *
 *  Returns     :   int, num node entries in list
 *
 *  Description :   Gets a count of the current list of instances.
 *
 ****************************************************************************/
int GetNumNodes( InstanceNode*       A_list)

{
    int            numNodes = 0;
    InstanceNode*  node;

    node = A_list;
	while (node)
    {
        numNodes++;
        node = node->Next;
    }
    
    return numNodes;
} 


/****************************************************************************
 *
 *  Function    :   GetNumOutstandingRequests
 *
 *  Parameters  :   A_list     - Pointer to head of node list to get count for
 *
 *  Returns     :   int, num messages we're waiting for c-store responses for
 *
 *  Description :   Checks the list of instances sent over the association &
 *                  returns the number of responses we're waiting for.
 *
 ****************************************************************************/
int GetNumOutstandingRequests(InstanceNode*       A_list)
{
    int            outstandingResponseMsgs = 0;
    InstanceNode*  node;

    node = A_list;
    while (node)
    {
        if ( ( node->imageSent == true )
          && ( node->responseReceived == false ) )
            outstandingResponseMsgs++;
        node = node->Next;
    }
    return outstandingResponseMsgs;
}    


/****************************************************************************
 *
 *  Function    :   SynchStorageCommitment
 *
 *  Parameters  :   commit_args includes:
 *                  options  - Pointer to structure containing input
 *                             parameters to the application
 *                  appID    - Application ID registered 
 *                  result   - List of objects to request commitment for.
 *
 *  Returns     :   
 *
 *  Description :   Perform synchronous storage commitment for a set of
 *                  storage objects. The list was created as the objects
 *                  themselves were sent.
 *
 ****************************************************************************/
void* SynchStorageCommitment(void* commit_args)
{
	MutexGuard guard (g_lock_sync_commit);

    int           associationID = -1;
    bool          sampStatus;
    NEVENT_ENUM   NEVENTStatus;
    MC_STATUS     mcStatus;
	COMMIT_ARGS*  commitArgs;

	commitArgs = (COMMIT_ARGS*)commit_args;

	if (!commitArgs->result->resultByFiles.length())
    {
        ::Message(MNOTE, toEndUser | toService | MLoverall, "No objects to commit.");

		guard.release();
		pthread_exit( (void *) &THREAD_NORMAL_EXIT );
    }

    /*
     * Open association to remote AE.  Instead of using the default service
     * list, use a special service list that only includes storage 
     * commitment.
     */
    mcStatus = MC_Open_Association( commitArgs->appID, &associationID,
                                    commitArgs->options.RemoteAE,
                                    commitArgs->options.RemotePort != -1 ? &commitArgs->options.RemotePort : NULL, 
                                    commitArgs->options.RemoteHostname[0] ? commitArgs->options.RemoteHostname : NULL,
                                    const_cast<char*>("Storage_Commit_SCU_Service_List") );
                                        
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        ::Message(MWARNING, toEndUser | toService | MLoverall, "Unable to open association with \"%s\":",commitArgs->options.RemoteAE);
        ::Message(MWARNING, toEndUser | toService | MLoverall, "\t%s", MC_Error_Message(mcStatus));

		guard.release();
		pthread_exit( (void *) &THREAD_EXCEPTION );
    }

    /*
     * Populate the N-ACTION message for storage commitment and sent it
     * over the network.  Also wait for a response message.
     */
    sampStatus = SetAndSendNActionMessage( commitArgs->options,
                                           associationID,
                                           commitArgs->result );
    if ( !sampStatus )
    {
        MC_Abort_Association(&associationID);

		guard.release();
		pthread_exit( (void *) &THREAD_EXCEPTION );
    }
   
    if (commitArgs->options.Verbose)
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Handle N-EVENT association in Synchronous Storage Commitment");


    /*
     * Handle the N-EVENT association.
     */
    NEVENTStatus = HandleNEventAssociation( commitArgs->options, associationID,								            
											commitArgs->result, DICOMStoragePkg::SYNCH_COMMIT );
    if ( NEVENTStatus != SUCCESS )
    {
        MC_Abort_Association(&associationID);

		guard.release();
		pthread_exit( (void *) &THREAD_EXCEPTION );
    }

    /*
     * When the close association fails, there's nothing really to be
     * done.
     */
    mcStatus = MC_Close_Association( &associationID);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("Close association failed", mcStatus);
        MC_Abort_Association(&associationID);
    }

	guard.release();
	pthread_exit( (void *) &THREAD_NORMAL_EXIT );

	return NULL;
} // end SynchStorageCommitment(...)


/****************************************************************************
 *
 *  Function    :   AsynchStorageCommitment
 *
 *  Parameters  :   commit_args includes:
 *                  options  - Pointer to structure containing input
 *                             parameters to the application
 *                  appID    - Application ID registered 
 *                  result   - List of objects to request commitment for.
 *
 *  Returns     :   
 *
 *  Description :   Perform asynchronous storage commitment for a set of
 *                  storage objects. The list was created as the objects
 *                  themselves were sent.
 *
 ****************************************************************************/
void* AsynchStorageCommitment(void* commit_args)
{
	MutexGuard guard (g_lock_async_commit);

    int           associationID = -1;
    int           calledApplicationID = -1;
    bool          sampStatus;
    NEVENT_ENUM   NEVENTStatus;
    MC_STATUS     mcStatus;
	COMMIT_ARGS*  commitArgs;
	int           cnt = 0, timeout_cnt = 0;

	commitArgs = (COMMIT_ARGS*)commit_args;

	if (!commitArgs->result->resultByFiles.length())
    {
        ::Message(MNOTE, toEndUser | toService | MLoverall, "No objects to commit.");

		guard.release();
		pthread_exit( (void *) &THREAD_NORMAL_EXIT );
    }

	/*
     * Open association to remote AE.  Instead of using the default service
     * list, use a special service list that only includes storage 
     * commitment.
     */
    mcStatus = MC_Open_Association( commitArgs->appID, &associationID,
                                    commitArgs->options.RemoteAE,
                                    commitArgs->options.RemotePort != -1 ? &commitArgs->options.RemotePort : NULL, 
                                    commitArgs->options.RemoteHostname[0] ? commitArgs->options.RemoteHostname : NULL,
                                    const_cast<char*>("Storage_Commit_SCU_Service_List") );
    
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        ::Message(MWARNING, toEndUser | toService | MLoverall, "Unable to open association with \"%s\":",commitArgs->options.RemoteAE);
        ::Message(MWARNING, toEndUser | toService | MLoverall, "\t%s", MC_Error_Message(mcStatus));

		guard.release();
		pthread_exit( (void *) &THREAD_EXCEPTION );
    }

    /*
     * Populate the N-ACTION message for storage commitment and sent it
     * over the network.  Also wait for a response message.
     */
    sampStatus = SetAndSendNActionMessage( commitArgs->options,
                                           associationID,
                                           commitArgs->result );
    if ( !sampStatus )
    {
        MC_Abort_Association(&associationID);

		guard.release();
		pthread_exit( (void *) &THREAD_EXCEPTION );
    }
    else
    {
        /*
         * When the close association fails, there's nothing really to be
         * done.  Let's still continue on and wait for an N-EVENT-REPORT
         */
        mcStatus = MC_Close_Association( &associationID);
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            PrintError("Close association failed", mcStatus);
            MC_Abort_Association(&associationID);
        }
    }
   
	guard.release(); // jhuang 10/22/08 for MLSDB 26711, release the lock here so we do not hold it during the loop

    if (commitArgs->options.Verbose)
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Waiting for N-EVENT-REPORT Association in Asynchronous Storage Commitment");

    for(cnt=0; cnt<=MAX_LOOP_ITERATIONS; cnt++)
    {
        /*
         * Wait as an SCU for an association from the storage commitment
         * SCP.  This association will contain an N-EVENT-REPORT-RQ message.
         */
#ifdef linux
		if (AsyncCommitIncomingPort == -1)
			mcStatus = MC_Wait_For_Association( "Storage_Commit_SCU_Service_List", 5,
				                                &calledApplicationID,
					                            &associationID);
		else
			mcStatus = MC_Wait_For_Association_On_Port( "Storage_Commit_SCU_Service_List", 5,
				                                calledApplicationID,
												AsyncCommitIncomingPort,
					                            &associationID);
#else
		mcStatus = MC_Wait_For_Association( "Storage_Commit_SCU_Service_List", 5,
			                                &calledApplicationID,
				                            &associationID);
#endif

        if (mcStatus == MC_TIMEOUT)
		{
			timeout_cnt++;
            continue;
		}
        else if (mcStatus == MC_UNKNOWN_HOST_CONNECTED)
        {
            ::Message(MWARNING, toEndUser | toService | MLoverall, "\tUnknown host connected, association rejected ");
			sleep(1);
            continue;
        }
        else if (mcStatus == MC_NEGOTIATION_ABORTED)
        {
            ::Message(MWARNING, toEndUser | toService | MLoverall, "\tAssociation aborted during negotiation ");
			sleep(1);
            continue;
        }
        else if (mcStatus != MC_NORMAL_COMPLETION)
        {
            PrintError("Error on MC_Wait_For_Association for asynch storage commitment",mcStatus);
			guard.release();
			pthread_exit( (void *) &THREAD_EXCEPTION );
        }
        
		if (commitArgs->options.Verbose)
	        ::Message(MNOTE, toEndUser | toService | MLoverall, "Accepting N-EVENT association");

		MutexGuard guardNEvent (g_lock_async_commit); // jhuang 10/22/08 for MLSDB 26711

		/*
	     * Accept the association.
		 */
	    mcStatus = MC_Accept_Association( associationID );
		if (mcStatus != MC_NORMAL_COMPLETION)
	    {
		    /*
			 * Make sure the association is cleaned up.
			 */
		    MC_Reject_Association( associationID,
			                       TRANSIENT_NO_REASON_GIVEN );
		    PrintError("Error on MC_Accept_Association", mcStatus);
			continue;
		}

        /*
         * Handle the N-EVENT association.  We are only expecting a single
         * association, so break out after we've handled the association.
         */
        NEVENTStatus = HandleNEventAssociation( commitArgs->options, associationID,
                                             commitArgs->result, DICOMStoragePkg::ASYNCH_COMMIT );
		guardNEvent.release(); // jhuang 10/22/08 for MLSDB 26711
		if ( NEVENTStatus != SUCCESS )
			pthread_exit( (void *) &THREAD_EXCEPTION );
		else
			break;
    }

	guard.release();
	if (cnt > MAX_LOOP_ITERATIONS)
	{
		if(timeout_cnt > MAX_LOOP_ITERATIONS)
		{
	        ::Message(MWARNING, toEndUser | toService | MLoverall, "Cstore cannot get asynchronous N-EVENT-REPORT call back from %s", commitArgs->options.RemoteHostname);
	        ::Message(MWARNING, toEndUser | toService | MLoverall, "Please make sure the camera incoming port %d is open. And make sure %s is asynchronous. If not, re-configure the camera to use synchronous commit and try again", commitArgs->options.RemotePort, commitArgs->options.RemoteHostname);
	        ::Message(MWARNING, toEndUser | toService | MLoverall, "Cstore is waiting for the next command.");

#ifdef DEBUG_PRINTF
	        printf("Cstore cannot get asynchronous N-EVENT-REPORT call back from %s\n", commitArgs->options.RemoteHostname);
	        printf("Please make sure the camera incoming port %d is open. And make sure %s is asynchronous. If not, re-configure the camera to use synchronous commit and try again\n", commitArgs->options.RemotePort, commitArgs->options.RemoteHostname);
	        printf("Cstore is waiting for the next command.\n");
#endif
		}
		pthread_exit( (void *) &THREAD_EXCEPTION );
	}
	else
		pthread_exit( (void *) &THREAD_NORMAL_EXIT );

	return NULL;
} // end AsynchStorageCommitment(...)


/****************************************************************************
 *
 *  Function    :   EitherStorageCommitment
 *
 *  Parameters  :   commit_args includes:
 *                  options  - Pointer to structure containing input
 *                             parameters to the application
 *                  appID    - Application ID registered 
 *                  result   - List of objects to request commitment for.
 *
 *  Returns     :   
 *
 *  Description :   Try to perform synchronous storage commitment for a set of
 *                  storage objects first. If timeout, automatically convert into
 *                  asynchronous storage commitment.
 *
 ****************************************************************************/
void* EitherStorageCommitment(void* commit_args)
{
	MutexGuard guard (g_lock_either_commit);

    int           associationID = -1;
    int           calledApplicationID = -1;
    bool          sampStatus;
    NEVENT_ENUM   NEVENTStatus;
    MC_STATUS     mcStatus;
	COMMIT_ARGS*  commitArgs;
	int           cnt = 0, timeout_cnt = 0;

	commitArgs = (COMMIT_ARGS*)commit_args;

	if (!commitArgs->result->resultByFiles.length())
    {
        ::Message(MNOTE, toEndUser | toService | MLoverall, "No objects to commit.");

		guard.release();
		pthread_exit( (void *) &THREAD_NORMAL_EXIT );
    }

    /*
     * Open association to remote AE. Instead of using the default service
     * list, use a special service list that only includes storage 
     * commitment.
     */
    mcStatus = MC_Open_Association( commitArgs->appID, &associationID,
                                    commitArgs->options.RemoteAE,
                                    commitArgs->options.RemotePort != -1 ? &commitArgs->options.RemotePort : NULL, 
                                    commitArgs->options.RemoteHostname[0] ? commitArgs->options.RemoteHostname : NULL,
                                    const_cast<char*>("Storage_Commit_SCU_Service_List") );
                                        
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        ::Message(MWARNING, toEndUser | toService | MLoverall, "Unable to open association with \"%s\":",commitArgs->options.RemoteAE);
        ::Message(MWARNING, toEndUser | toService | MLoverall, "\t%s", MC_Error_Message(mcStatus));

		guard.release();
		pthread_exit( (void *) &THREAD_EXCEPTION );
    }

    /*
     * Populate the N-ACTION message for storage commitment and sent it
     * over the network.  Also wait for a response message.
     */
    sampStatus = SetAndSendNActionMessage( commitArgs->options,
                                           associationID,
                                           commitArgs->result );
    if ( !sampStatus )
    {
        MC_Abort_Association(&associationID);

		guard.release();
		pthread_exit( (void *) &THREAD_EXCEPTION );
    }
   
    if (commitArgs->options.Verbose)
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Handle N-EVENT association.");


    /*
     * Handle the N-EVENT association.
     */
    NEVENTStatus = HandleNEventAssociation( commitArgs->options, associationID,								            
											commitArgs->result, DICOMStoragePkg::EITHER_COMMIT );
    if ( NEVENTStatus == FAILURE )
    {
        MC_Abort_Association(&associationID);

		guard.release();
		pthread_exit( (void *) &THREAD_EXCEPTION );
    }
	else if ( NEVENTStatus == SUCCESS )
	{
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Cstore detected and accomplished Synchronous Storage Commitment from remote site %s", commitArgs->options.RemoteHostname);
		/*
		 * When the close association fails, there's nothing really to be
		 * done.
		 */
		mcStatus = MC_Close_Association( &associationID);
	    if (mcStatus != MC_NORMAL_COMPLETION)
		{
			PrintError("Close association failed", mcStatus);
		    MC_Abort_Association(&associationID);
		}
	}
	else // TIMEOUT. Automatically convert into asynchronous commitment
	{
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Synchronous Storage Commitment timed out from remote site %s. Automatically convert into Asynchronous Storage Commitment.", commitArgs->options.RemoteHostname);
        /*
         * Let's still continue on and wait for an N-EVENT-REPORT asynchronously
         */
        mcStatus = MC_Close_Association( &associationID);
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            PrintError("Close association failed", mcStatus);
            MC_Abort_Association(&associationID);
        }
   
		guard.release(); // jhuang 10/22/08 for MLSDB 26711, release the lock here so we do not hold it during the loop

        if (commitArgs->options.Verbose)
            ::Message(MNOTE, toEndUser | toService | MLoverall, "Waiting for N-EVENT-REPORT Association in Asynchronous Storage Commitment");

	    for(cnt=0; cnt<=MAX_LOOP_ITERATIONS; cnt++)
        {
            /*
             * Wait as an SCU for an association from the storage commitment
             * SCP.  This association will contain an N-EVENT-REPORT-RQ message.
             */
#ifdef linux
			if (AsyncCommitIncomingPort == -1)
				mcStatus = MC_Wait_For_Association( "Storage_Commit_SCU_Service_List", 5,
					                                &calledApplicationID,
						                            &associationID);
			else
				mcStatus = MC_Wait_For_Association_On_Port( "Storage_Commit_SCU_Service_List", 5,
					                                calledApplicationID,
													AsyncCommitIncomingPort,
						                            &associationID);
#else
			mcStatus = MC_Wait_For_Association( "Storage_Commit_SCU_Service_List", 5,
				                                &calledApplicationID,
					                            &associationID);
#endif
            if (mcStatus == MC_TIMEOUT)
			{
				timeout_cnt++;
                continue;
			}
            else if (mcStatus == MC_UNKNOWN_HOST_CONNECTED)
            {
                ::Message(MWARNING, toEndUser | toService | MLoverall, "\tUnknown host connected, association rejected ");
				sleep(1);
                continue;
            }
            else if (mcStatus == MC_NEGOTIATION_ABORTED)
            {
                ::Message(MWARNING, toEndUser | toService | MLoverall, "\tAssociation aborted during negotiation ");
				sleep(1);
                continue;
            }
            else if (mcStatus != MC_NORMAL_COMPLETION)
            {
                PrintError("Error on MC_Wait_For_Association for asynch storage commitment",mcStatus);
		    	guard.release();
		    	pthread_exit( (void *) &THREAD_EXCEPTION );
            }
        
	    	if (commitArgs->options.Verbose)
	            ::Message(MNOTE, toEndUser | toService | MLoverall, "Accepting N-EVENT association");

			MutexGuard guardNEvent (g_lock_async_commit); // jhuang 10/22/08 for MLSDB 26711

	    	/*
	         * Accept the association.
		     */
	        mcStatus = MC_Accept_Association( associationID );
	    	if (mcStatus != MC_NORMAL_COMPLETION)
	        {
	    	    /*
		    	 * Make sure the association is cleaned up.
		    	 */
		        MC_Reject_Association( associationID,
			                           TRANSIENT_NO_REASON_GIVEN );
		        PrintError("Error on MC_Accept_Association", mcStatus);
			    continue;
		    }

            /*
             * Handle the N-EVENT association.  We are only expecting a single
             * association, so break out after we've handled the association.
             */
            NEVENTStatus = HandleNEventAssociation( commitArgs->options, associationID,
                                                 commitArgs->result, DICOMStoragePkg::ASYNCH_COMMIT );
			guardNEvent.release(); // jhuang 10/22/08 for MLSDB 26711
		    if ( NEVENTStatus != SUCCESS )
		    	pthread_exit( (void *) &THREAD_EXCEPTION );
			else
			{
				::Message(MWARNING, toEndUser | toService | MLoverall, "Cstore detected asynchronous commit from %s. Please re-configure the camera to use asynchronous for this host as this will save significant amount of time for each storage commitment", commitArgs->options.RemoteHostname);
#ifdef DEBUG_PRINTF
				printf("Cstore detected asynchronous commit from %s. Please re-configure the camera to use asynchronous for this host as this will save significant amount of time for each storage commitment\n", commitArgs->options.RemoteHostname);
#endif
			    break;
			}
        }
	}

	guard.release();
	if (cnt > MAX_LOOP_ITERATIONS)
	{
		if(timeout_cnt > MAX_LOOP_ITERATIONS)
		{
	        ::Message(MWARNING, toEndUser | toService | MLoverall, "Cstore cannot get synchronous or asynchronous N-EVENT-REPORT from %s", commitArgs->options.RemoteHostname);
	        ::Message(MWARNING, toEndUser | toService | MLoverall, "Please double check %s is running and re-configure the camera to use the exact synchronous or asynchronous commitment", commitArgs->options.RemoteHostname);
	        ::Message(MWARNING, toEndUser | toService | MLoverall, "Cstore is waiting for the next command.");

#ifdef DEBUG_PRINTF
	        printf("Cstore cannot get synchronous or asynchronous N-EVENT-REPORT from %s\n", commitArgs->options.RemoteHostname);
	        printf("Please double check %s is running and re-configure the camera to use the exact synchronous or asynchronous commit\n", commitArgs->options.RemoteHostname);
	        printf("Cstore is waiting for the next command.\n");
#endif
		}
		pthread_exit( (void *) &THREAD_EXCEPTION );
	}
	else
		pthread_exit( (void *) &THREAD_NORMAL_EXIT );

	return NULL;
} // end EitherStorageCommitment(...)

/****************************************************************************
 *
 *  Function    :   SetAndSendNActionMessage
 *
 *  Parameters  :   A_options  - Reference to structure containing input
 *                               parameters to the application
 *                  A_associationID - Association ID registered 
 *                  A_result   - List of objects to request commitment for.
 *
 *  Returns     :   true
 *                  false
 *
 *  Description :   Populate an N-ACTION-RQ message to be sent, and wait
 *                  for a response to the request.
 *
 ****************************************************************************/
bool SetAndSendNActionMessage(
                        COMMIT_OPTIONS&                         A_options,
                        int                                     A_associationID,
                        DICOMStoragePkg::ResultByStorageTarget* A_result)
{
    MC_STATUS      mcStatus;
    int            messageID;
    int            itemID;
    unsigned int   i;
    int            responseMessageID;
    char*          transactionUID;
    char*          responseService;
    MC_COMMAND     responseCommand;
    int            responseStatus;

    mcStatus = MC_Open_Message( &messageID, "STORAGE_COMMITMENT_PUSH",
                                N_ACTION_RQ );
    if ( mcStatus != MC_NORMAL_COMPLETION )
    {
        PrintError("Error opening Storage Commitment message",mcStatus);
        return ( false );
    }

    /*
     * Set the well-known SOP instance for storage commitment Push, as
     * listed in DICOM PS3.4, J.3.5
     */
    mcStatus = MC_Set_Value_From_String( messageID, 
                                         MC_ATT_REQUESTED_SOP_INSTANCE_UID,
                                         "1.2.840.10008.1.20.1.1" );
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Set_Value_From_String for requested sop instance uid failed",mcStatus);
        MC_Free_Message(&messageID);
        return ( false );
    }

    /*
     * Action ID as defined in DICOM PS3.4
     */
    mcStatus = MC_Set_Next_Value_From_Int( messageID,
                                           MC_ATT_ACTION_TYPE_ID,
                                           1 );
    if ( mcStatus != MC_NORMAL_COMPLETION )
    {
        PrintError("Unable to set ItemID in n-action message", mcStatus);
        MC_Free_Message( &messageID );
        return ( false );
    }

    /*
     * Set the transaction UID.  This UID should be tracked and associated
     * with the SOP instances asked for commitment with this request.
     * That way if multiple storage commitment requests are outstanding,
     * and an N-EVENT-REPORT comes in, we can associate the message with
     * the proper storage commitment request.  Commitment or commitment 
     * failure for specific objects can then be tracked.
     */
    transactionUID = Create_Inst_UID();
	A_result->transactionUID = CORBA::string_dup(transactionUID);
    mcStatus = MC_Set_Value_From_String( messageID, 
                                         MC_ATT_TRANSACTION_UID,
                                         transactionUID );
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Set_Value_From_String for transaction uid failed",mcStatus);
        MC_Free_Message(&messageID);
        return ( false );
    }

    if (A_options.Verbose)
        ::Message(MNOTE, toEndUser | toService | MLoverall, 
				"\nSending N-Action with transaction UID: %s", transactionUID);

    /*
     * Create an item for each SOP instance we are asking commitment for.
     * The item contains the SOP Class & Instance UIDs for the object.
     */
	for (i=0; i<A_result->resultByFiles.length(); i++)
    {
		if (!strlen(A_result->resultByFiles[i].SOPClassUID.in()) ||
			!strlen(A_result->resultByFiles[i].SOPInstanceUID.in()))
			continue;

        mcStatus = MC_Open_Item( &itemID,
                                 "REF_SOP_MEDIA" );
        if ( mcStatus != MC_NORMAL_COMPLETION )
        {
            MC_Free_Item( &itemID );
            MC_Free_Message( &messageID );
            return ( false );
        }

        /*
         * Set_Next_Value so that we can set multiple items within
         * the sequence attribute.
         */
        mcStatus = MC_Set_Next_Value_From_Int( messageID,
                                               MC_ATT_REFERENCED_SOP_SEQUENCE,
                                               itemID );
        if ( mcStatus != MC_NORMAL_COMPLETION )
        {
            PrintError("Unable to set ItemID in n-action message", mcStatus);
            MC_Free_Item( &itemID );
            MC_Free_Message( &messageID );
            return ( false );
        }
        
        mcStatus = MC_Set_Value_From_String( itemID,
                                             MC_ATT_REFERENCED_SOP_CLASS_UID,
                                             A_result->resultByFiles[i].SOPClassUID.in());
        if ( mcStatus != MC_NORMAL_COMPLETION )
        {
            PrintError("Unable to set SOP Class UID in n-action message", mcStatus);
            MC_Free_Item( &itemID );
            MC_Free_Message( &messageID );
            return ( false );
        }
        
        mcStatus = MC_Set_Value_From_String( itemID,
                                             MC_ATT_REFERENCED_SOP_INSTANCE_UID,
                                             A_result->resultByFiles[i].SOPInstanceUID.in());
        if ( mcStatus != MC_NORMAL_COMPLETION )
        {
            PrintError("Unable to set SOP Instance UID in n-action message", mcStatus);
            MC_Free_Item( &itemID );
            MC_Free_Message( &messageID );
            return ( false );
        }
        
        if (A_options.Verbose)
        {
            ::Message(MNOTE, toEndUser | toService | MLoverall, "   Object SOP Class UID: %s", A_result->resultByFiles[i].SOPClassUID.in());
            ::Message(MNOTE, toEndUser | toService | MLoverall, "Object SOP Instance UID: %s\n", A_result->resultByFiles[i].SOPInstanceUID.in());
        }

        MC_Free_Item( &itemID );
    }
    

    /*
     * Once the message has been built, we are then able to perform the
     * N-ACTION-RQ on it.
     */
    mcStatus = MC_Send_Request_Message( A_associationID, messageID );
    if ( mcStatus != MC_NORMAL_COMPLETION )
    {
        PrintError("Unable to send N-ACTION-RQ message",mcStatus);
        MC_Free_Message( &messageID );
        return ( false );
    }

    /*
     * After sending the message, we free it.
     */
    mcStatus = MC_Free_Message( &messageID );
    if ( mcStatus != MC_NORMAL_COMPLETION )
    {
        PrintError("Unable to free N-ACTION-RQ message",mcStatus);
        return ( false );
    }
    

    /*
     *  Wait for N-ACTION-RSP message.
     */
    mcStatus = MC_Read_Message(A_associationID, 300, &responseMessageID,
                 &responseService, &responseCommand);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Read_Message failed for N-ACTION-RSP", mcStatus);
        return ( false );
    }

    /*
     * Check the status in the response message.
     */
    mcStatus = MC_Get_Value_To_Int(responseMessageID, MC_ATT_STATUS, &responseStatus);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Get_Value_To_Int for N-ACTION-RSP status failed",mcStatus);
        MC_Free_Message(&responseMessageID);
        return ( false );
    }

    switch (responseStatus)
    {
        case N_ACTION_NO_SUCH_SOP_INSTANCE:
            ::Message(MWARNING, toEndUser | toService | MLoverall, "N-ACTION-RSP failed because of invalid SOP INSTANCE UID");
            MC_Free_Message(&responseMessageID);
            return ( false );
        case N_ACTION_PROCESSING_FAILURE:
            ::Message(MWARNING, toEndUser | toService | MLoverall, "N-ACTION-RSP failed because of processing failure");
            MC_Free_Message(&responseMessageID);
            return ( false );
        case N_ACTION_SUCCESS:
            ::Message(MNOTE, toEndUser | toService | MLoverall, "Cstore received N-ACTION-RSP successfully.");
            break;
            
        default:
            ::Message(MWARNING, toEndUser | toService | MLoverall, "N-ACTION-RSP failure, status=0x%x",responseStatus);
            MC_Free_Message(&responseMessageID);
            return ( false );
    }

    mcStatus = MC_Free_Message( &responseMessageID );
    if ( mcStatus != MC_NORMAL_COMPLETION )
    {
        PrintError("Unable to free N-ACTION-RSP message",mcStatus);
        return ( false );
    }

    
    return( true );

} /* End of SetAndSendNActionMessage */



/****************************************************************************
 *
 *  Function    :   HandleNEventAssociation
 *
 *  Parameters  :   A_options  - Reference to structure containing input
 *                               parameters to the application
 *                  A_associationID - Association ID registered 
 *                  A_result   - List of objects to request commitment for.
 *
 *  Returns     :   true
 *                  false
 *
 *  Description :   Handle a storage commitment association when expecting 
 *                  an N-EVENT-REPORT-RQ message.
 *
 ****************************************************************************/
NEVENT_ENUM HandleNEventAssociation(
                        COMMIT_OPTIONS&                         A_options,
                        int                                     A_associationID, 
                        DICOMStoragePkg::ResultByStorageTarget* A_result,
                        DICOMStoragePkg::CommitType			    A_commitType)
{
    MC_STATUS     mcStatus;
    bool          sampStatus;
    RESP_STATUS   respStatus;
    int           messageID;
    int           rspMessageID;
    MC_COMMAND    command;
    char*         serviceName;
    int           cnt;

    ::Message(MNOTE, toEndUser | toService | MLoverall, "Cstore sets Role Reversal WaitTime to be %d seconds.", A_options.RoleReversalWaitTime);
    for (cnt=0; cnt<=MAX_LOOP_ITERATIONS; cnt++)
    {
        /*
         * Note, only the requestor of an association can close the association.
         * So for asynchronous commitment, we wait here in the read message call
		 * after we have received the N-EVENT-REPORT for the connection to close.
         */
        mcStatus = MC_Read_Message( A_associationID, 
                                    A_options.RoleReversalWaitTime,
                                    &messageID,
                                    &serviceName, 
                                    &command);
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            if (mcStatus == MC_TIMEOUT)
            {
                if(A_commitType == DICOMStoragePkg::EITHER_COMMIT)
				{
					::Message(MNOTE, toEndUser | toService | MLoverall, "Synchronous commit timed out. Now automatically convert into Asynchronous commit.");
					return ( TIMEOUT );
				}

                ::Message(MNOTE, toEndUser | toService | MLoverall, "Timeout occurred waiting for message.  Waiting again.");
				continue;
            }
            else if (mcStatus == MC_ASSOCIATION_CLOSED)
            {       
                ::Message(MNOTE, toEndUser | toService | MLoverall, "Association Closed.");
				return ( SUCCESS );
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
                PrintError("Unexpected event in HandleNEventAssociation, association aborted", mcStatus);
				return ( FAILURE ); 
            }
            
            PrintError("Error on MC_Read_Message in HandleNEventAssociation", mcStatus);
            MC_Abort_Association(&A_associationID);
			return ( FAILURE ); 
        }

        sampStatus = ProcessNEventMessage(messageID, A_result);
        if (sampStatus == true )
			respStatus = N_EVENT_SUCCESS;
        else
            respStatus = N_EVENT_PROCESSING_FAILURE; 


        mcStatus = MC_Free_Message(&messageID);
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            PrintError("MC_Free_Message of PRINTER,N_EVENT_REPORT_RSP error",mcStatus);
            return ( FAILURE ); 
        } 

        /*
         * Now lets send a response message.
         */
        mcStatus = MC_Open_Message ( &rspMessageID, 
                                     "STORAGE_COMMITMENT_PUSH", 
                                     N_EVENT_REPORT_RSP );
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            PrintError("MC_Open_Message error of N-EVENT response",mcStatus);
            return ( FAILURE ); 
        }

        mcStatus = MC_Send_Response_Message( A_associationID,  
                                             respStatus, 
                                             rspMessageID );
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            PrintError("MC_Send_Response_Message for N_EVENT_REPORT_RSP error",mcStatus);
            MC_Free_Message(&rspMessageID);
            return( FAILURE ); 
        }

        mcStatus = MC_Free_Message(&rspMessageID);
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            PrintError("MC_Free_Message of PRINTER,N_EVENT_REPORT_RSP error",mcStatus);
            return ( FAILURE ); 
        }

		/*
		 * For synchronous commitment, we break out here.
		 * If an either-commitment gets this far, it must be synchronous.
         */
		if (A_commitType == DICOMStoragePkg::SYNCH_COMMIT || 
			A_commitType == DICOMStoragePkg::EITHER_COMMIT)
			break;
	}

	if (cnt > MAX_LOOP_ITERATIONS)
		return ( FAILURE ); 
	else
		return ( SUCCESS );
} // end of HandleNEventAssociation


/****************************************************************************
 *
 *  Function    :   ProcessNEventMessage
 *
 *  Parameters  :   A_messageID - Association ID registered 
 *                  A_result    - List of objects to request commitment for.
 *
 *  Returns     :   true
 *                  false
 *
 *  Description :   Handle a storage commitment association when expecting 
 *                  an N-EVENT-REPORT-RQ message.
 *
 ****************************************************************************/
bool ProcessNEventMessage(
                        int                    A_messageID, 
                        DICOMStoragePkg::ResultByStorageTarget* A_result)
{
    char           uidBuffer[UI_LENGTH+2];
    char           sopClassUID[UI_LENGTH+2];
    char           sopInstanceUID[UI_LENGTH+2];
    unsigned short eventType;
    MC_STATUS      mcStatus;
    int            itemID;
    int            index;

    mcStatus = MC_Get_Value_To_String( A_messageID, 
                    MC_ATT_TRANSACTION_UID, 
                    sizeof(uidBuffer), uidBuffer);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("Unable to retreive transaction UID", mcStatus);
        uidBuffer[0] = '\0';
        return false;
    }

    /* 
     * At this time, the transaction UID is compared to a 
     * transaction UID of a previous storage commitment request.
     */
	if(strcmp(A_result->transactionUID.in(), uidBuffer))
	{
		::Message(MWARNING, toEndUser | toService | MLoverall, 
			"Different transactionUID for storage commitment: what got from N-EVENT '%s' is different from expected: '%s'.",
			uidBuffer, A_result->transactionUID.in());
		return false;
		// work here: may need to put the uidBuffer back to the pipe?
	}
	else
		::Message(MNOTE, toEndUser | toService | MLoverall, 
			"TransactionUID matches for storage commitment: what got from N-EVENT '%s' is the same as expected: '%s'.",
			uidBuffer, A_result->transactionUID.in());

    /* 
     * The following code can then compare the successful SOP 
     * instances and failed SOP instances with those that were
     * requested.
     */
	
    mcStatus = MC_Get_Value_To_UShortInt( A_messageID, 
                                          MC_ATT_EVENT_TYPE_ID, 
                                          &eventType );
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("Unable to retreive event type ID", mcStatus);
        eventType = 0;
    }

    switch( eventType )
    {
        case 1: /* SUCCESS */
            ::Message(MNOTE, toEndUser | toService | MLoverall, "\nCommit for N-EVENT Transaction UID: %s succeeded", uidBuffer );
            
            break;
        case 2: /* FAILURE */
            ::Message(MNOTE, toEndUser | toService | MLoverall, "\nCommit for N-EVENT Transaction UID: %s failed", uidBuffer );
            /*
             * At this point, the failure list is traversed through
             * to determine which images failed for the transaction.
             * This should be compared to the originals.
             */
            ::Message(MWARNING, toEndUser | toService | MLoverall, "    Failed to commit SOP Instances:");

            mcStatus = MC_Get_Next_Value_To_Int( A_messageID,
                                                 MC_ATT_FAILED_SOP_SEQUENCE,
                                                 &itemID );
            while ( mcStatus == MC_NORMAL_COMPLETION )
            {
                mcStatus = MC_Get_Value_To_String( itemID,
                                                   MC_ATT_REFERENCED_SOP_CLASS_UID,
                                                   sizeof(sopClassUID),
                                                   sopClassUID );
                if ( mcStatus != MC_NORMAL_COMPLETION )
                {
                    PrintError("Unable to get SOP Class UID in n-event-rq message", mcStatus);
                    sopClassUID[0] = '\0';
                }

                mcStatus = MC_Get_Value_To_String( itemID,
                                                   MC_ATT_REFERENCED_SOP_INSTANCE_UID,
                                                   sizeof(sopInstanceUID),
                                                   sopInstanceUID );
                if ( mcStatus != MC_NORMAL_COMPLETION )
                {
                    PrintError("Unable to get SOP Instance UID in n-event-rq message", mcStatus);
                    sopInstanceUID[0] = '\0';
                }

                ::Message(MNOTE, toEndUser | toService | MLoverall, "ProcessNEventMessage gets    SOP Class UID: %s", sopClassUID );
                ::Message(MNOTE, toEndUser | toService | MLoverall, "ProcessNEventMessage gets SOP Instance UID: %s\n", sopInstanceUID );

				index = findNode(A_result->resultByFiles, sopClassUID, sopInstanceUID);
				if (index>=0)
				{
					A_result->resultByFiles[index].commitOutcome = DICOMStoragePkg::COMMIT_FAILURE;
					::Message(MWARNING, toEndUser | toService | MLoverall, "COMMIT_FAILURE for %s.", A_result->resultByFiles[index].imgFile.in());
#ifdef DEBUG_PRINTF
					printf("COMMIT_FAILURE for %s.\n", A_result->resultByFiles[index].imgFile.in());
#endif
				}
				else
					::Message( MWARNING, toEndUser | toService | MLoverall, 
							"Storage commit failed for sopClassUID: %s, sopInstanceUID: %s. But could not find the corresponding image file.",
							sopClassUID, sopInstanceUID);

                mcStatus = MC_Get_Next_Value_To_Int( A_messageID,
                                                     MC_ATT_FAILED_SOP_SEQUENCE,
                                                     &itemID );
            }

            break;
        default:
            ::Message(MWARNING, toEndUser | toService | MLoverall,  "Transaction UID: %s event_report invalid event type %d",
                    uidBuffer, eventType );
    }

        
    /*
     * We compare here the original SOP instances in the 
     * original transaction to what was returned here.
     */
    ::Message(MNOTE, toEndUser | toService | MLoverall, "    Successfully committed SOP Instances:");

    mcStatus = MC_Get_Next_Value_To_Int( A_messageID,
                                         MC_ATT_REFERENCED_SOP_SEQUENCE,
                                         &itemID );
    while ( mcStatus == MC_NORMAL_COMPLETION )
    {
        mcStatus = MC_Get_Value_To_String( itemID,
                                           MC_ATT_REFERENCED_SOP_CLASS_UID,
                                           sizeof(sopClassUID),
                                           sopClassUID );
        if ( mcStatus != MC_NORMAL_COMPLETION )
        {
            PrintError("Unable to get SOP Class UID in n-event-rq message", mcStatus);
            sopClassUID[0] = '\0';
        }

        mcStatus = MC_Get_Value_To_String( itemID,
                                           MC_ATT_REFERENCED_SOP_INSTANCE_UID,
                                           sizeof(sopInstanceUID),
                                           sopInstanceUID );
        if ( mcStatus != MC_NORMAL_COMPLETION )
        {
            PrintError("Unable to get SOP Instance UID in n-event-rq message", mcStatus);
            sopInstanceUID[0] = '\0';
        }

        ::Message(MNOTE, toEndUser | toService | MLoverall, "       SOP Class UID: %s", sopClassUID );
        ::Message(MNOTE, toEndUser | toService | MLoverall, "    SOP Instance UID: %s", sopInstanceUID );

		index = findNode(A_result->resultByFiles, sopClassUID, sopInstanceUID);
		if (index>=0)
		{
			A_result->resultByFiles[index].commitOutcome = DICOMStoragePkg::COMMIT_SUCCEESS;
			::Message(MNOTE, toEndUser | toService | MLoverall, "COMMIT_SUCCEESS for %s.", A_result->resultByFiles[index].imgFile.in());
#ifdef DEBUG_PRINTF
			printf("COMMIT_SUCCEESS for %s.\n", A_result->resultByFiles[index].imgFile.in());
#endif
		}
		else
			::Message( MWARNING, toEndUser | toService | MLoverall, 
					"Storage commit succeeded for sopClassUID: %s, sopInstanceUID: %s. But could not find the corresponding image file.",
					sopClassUID, sopInstanceUID);

        mcStatus = MC_Get_Next_Value_To_Int( A_messageID,
                                             MC_ATT_REFERENCED_SOP_SEQUENCE,
                                             &itemID );
    }
    
    return ( true );
}  // ProcessNEventMessage


/****************************************************************************
 *
 *  Function    :   ReadImage
 *
 *  Parameters  :   A_options  - Reference to structure containing input
 *                               parameters to the application
 *                  A_appID    - Application ID registered 
 *                  A_node     - The node in our list of instances
 *
 *  Returns     :   true
 *                  false
 *
 *  Description :   Determine the format of a DICOM file and read it into
 *                  memory.  Note that in a production application, the 
 *                  file format should be predetermined (and not have to be
 *                  "guessed" by the CheckFileFormat function).  The 
 *                  format for this application was chosen to show how both
 *                  DICOM Part 10 format files and "stream" format objects
 *                  can be sent over the network.
 *
 ****************************************************************************/
bool ReadImage( STORAGE_OPTIONS&  A_options,
                int               A_appID, 
                InstanceNode*     A_node)
{
    FORMAT_ENUM             format = UNKNOWN_FORMAT;
    bool                    sampBool = false;
    MC_STATUS               mcStatus;


    format = CheckFileFormat( A_node->fname );
    switch(format)
    {
        case MEDIA_FORMAT:
            A_node->mediaFormat = true;
            sampBool = ReadFileFromMedia( A_options, 
                                          A_appID, 
                                          A_node->fname, 
                                          &A_node->msgID, 
                                          &A_node->transferSyntax, 
                                          &A_node->imageBytes );
            break;
        
        case IMPLICIT_LITTLE_ENDIAN_FORMAT:
        case IMPLICIT_BIG_ENDIAN_FORMAT:
        case EXPLICIT_LITTLE_ENDIAN_FORMAT:
        case EXPLICIT_BIG_ENDIAN_FORMAT:
            A_node->mediaFormat = false;
            sampBool = ReadMessageFromFile( A_options, 
                                            A_node->fname, 
                                            format, 
                                            &A_node->msgID, 
                                            &A_node->transferSyntax, 
                                            &A_node->imageBytes );
            break;
            
        case UNKNOWN_FORMAT:
            PrintError("Unable to determine the format of file",
                       MC_NORMAL_COMPLETION);
            sampBool = false;
            break;
    }
    if ( sampBool == true )
    {
        mcStatus = MC_Get_Value_To_String(A_node->msgID, 
                        MC_ATT_SOP_CLASS_UID,
                        sizeof(A_node->SOPClassUID),
                        A_node->SOPClassUID);
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            PrintError("MC_Get_Value_To_String for SOP Class UID failed", mcStatus);
        }

        mcStatus = MC_Get_Value_To_String(A_node->msgID, 
                        MC_ATT_SOP_INSTANCE_UID,
                        sizeof(A_node->SOPInstanceUID),
                        A_node->SOPInstanceUID);
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            PrintError("MC_Get_Value_To_String for SOP Instance UID failed", mcStatus);
        }

        if (A_options.Verbose)
        {
            ::Message(MNOTE, toEndUser | toService | MLoverall, "ReadImage gets    SOP Class UID: %s", A_node->SOPClassUID );
            ::Message(MNOTE, toEndUser | toService | MLoverall, "ReadImage gets SOP Instance UID: %s\n", A_node->SOPInstanceUID );
        }
    }

    return sampBool;
}    


/****************************************************************************
 *
 *  Function    :   SendImage
 *
 *  Parameters  :   A_options  - Reference to structure containing input
 *                               parameters to the application
 *                  A_associationID    - Association ID 
 *                  A_node     - The node in our list of instances
 *
 *  Returns     :   true
 *                  false on failure where association must be aborted
 *
 *  Description :   Send one image over the network.
 *
 *                  true is returned on success, or when a recoverable
 *                  error occurs.
 *
 ****************************************************************************/
bool SendImage(               STORAGE_OPTIONS&  A_options,
                              int               A_associationID, 
                              InstanceNode*     A_node)
{
    MC_STATUS       mcStatus;

    A_node->imageSent = false;
    
    /* Get the SOP class UID and set the service */
    mcStatus = MC_Get_MergeCOM_Service(A_node->SOPClassUID, A_node->serviceName, sizeof(A_node->serviceName));
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Get_MergeCOM_Service failed", mcStatus);
        return ( true );
    }            
                 
    mcStatus = MC_Set_Service_Command(A_node->msgID, A_node->serviceName, C_STORE_RQ);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Set_Service_Command failed", mcStatus);
        return ( true );
    }
            
    /* set affected SOP Instance UID */
    mcStatus = MC_Set_Value_From_String(A_node->msgID, 
                      MC_ATT_AFFECTED_SOP_INSTANCE_UID,
                      A_node->SOPInstanceUID);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Set_Value_From_String failed for affected SOP Instance UID", mcStatus);
        return ( true );
    }
            
    /*
     *  Send the message
     */
    if (A_options.Verbose)
    {
        ::Message(MNOTE, toEndUser | toService | MLoverall, "     File: %s", A_node->fname);
        if ( A_node->mediaFormat )
			::Message(MNOTE, toEndUser | toService | MLoverall, "   Format: DICOM Part 10 Format(%s)", GetSyntaxDescription(A_node->transferSyntax));
        else
			::Message(MNOTE, toEndUser | toService | MLoverall, "   Format: Stream Format(%s)", GetSyntaxDescription(A_node->transferSyntax));
        ::Message(MNOTE, toEndUser | toService | MLoverall, "   SOP Class UID: %s (%s)", A_node->SOPClassUID, A_node->serviceName);
        ::Message(MNOTE, toEndUser | toService | MLoverall, "SOP Instance UID: %s", A_node->SOPInstanceUID);
        ::Message(MNOTE, toEndUser | toService | MLoverall, "     Size: %lu bytes", (long)A_node->imageBytes);
    }

    mcStatus = MC_Send_Request_Message(A_associationID, A_node->msgID);
    if (mcStatus == MC_ASSOCIATION_ABORTED || mcStatus == MC_SYSTEM_ERROR)
    {
        /*
         * At this point, the association has been dropped, or we should
         * drop it in the case of MC_SYSTEM_ERROR.
         */
        PrintError("MC_Send_Request_Message failed", mcStatus);
        return ( false );
    }
#ifdef linux
    else if (mcStatus == MC_MAX_OPERATIONS_EXCEEDED)
	{	PrintError("Warning: MC_Send_Request_Message would cause the Max # of Operations Invoked that was negotiated to be exceeded. A response message must be read before this message can be sent.",
			mcStatus);
        return ( false );
	}
#endif
    else if (mcStatus != MC_NORMAL_COMPLETION)
    {
        /*
         * This is a failure condition we can continue with
         */
        PrintError("Warning: MC_Send_Request_Message failed", mcStatus);
        return ( true );
    }

    A_node->imageSent = true;

    return ( true );
}    


/****************************************************************************
 *
 *  Function    :   ReadResponseMessages
 *
 *  Parameters  :   A_options  - Reference to structure containing input
 *                               parameters to the application
 *
 *  Returns     :   true
 *                  false on failure where association must be aborted
 *
 *  Description :   Process C-STORE-RQ response message.
 *
 *                  true is returned on success, or when a recoverable
 *                  error occurs.
 *
 ****************************************************************************/
bool ReadResponseMessages(    STORAGE_OPTIONS&  A_options,
                              int               A_associationID, 
                              int               A_timeout,
                              InstanceNode**    A_list)
{
    MC_STATUS       mcStatus;
    bool            sampBool;
    int             responseMessageID;
    char*           responseService;
    MC_COMMAND      responseCommand;
    unsigned int    dicomMsgID;
    InstanceNode*   node;
    
    /*
     *  Wait for response
     */
    mcStatus = MC_Read_Message(A_associationID, A_timeout, &responseMessageID,
                 &responseService, &responseCommand);
    if (mcStatus == MC_TIMEOUT)
        return ( true );
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Read_Message failed", mcStatus);
        return ( false );
    }
    
    mcStatus = MC_Get_Value_To_UInt(responseMessageID, 
                    MC_ATT_MESSAGE_ID_BEING_RESPONDED_TO,
                    &dicomMsgID);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Get_Value_To_UInt for Message ID Being Responded To failed.  Unable to process response message.", mcStatus);
        return(true);
    }
    
    node = *A_list;
    while (node)
    {
        if ( node->dicomMsgID == dicomMsgID )
        {
            break;
        }
        node = node->Next;
    }
   
    if ( !node )
    {
        ::Message(MWARNING, toEndUser | toService | MLoverall,  "Message ID Being Responded To tag does not match message sent over association: %d", dicomMsgID );
        MC_Free_Message(&responseMessageID);
        return ( true );
    }
   
    node->responseReceived = true;
        
    sampBool = CheckResponseMessage ( responseMessageID, node );
    if (!sampBool)
    {
        node->failedResponse = true;
    }
    
    ::Message(MNOTE, toEndUser | toService | MLoverall, "Storage Status: file %s %s", node->fname, node->statusMeaning);
        
    node->failedResponse = false;

    mcStatus = MC_Free_Message(&responseMessageID);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Free_Message failed for response message", mcStatus);
        return ( true );
    }

    return ( true );
}


/****************************************************************************
 *
 *  Function    :   CheckResponseMessage
 *
 *  Parameters  :   A_responseMsgID  - The message ID of the response message
 *                                     for which we want to check the status
 *                                     tag.
 *
 *  Returns     :   true on success or warning status
 *                  false on failure status
 *
 *  Description :   Examine the status tag in the response to see if
 *                  the C-STORE-RQ was successfully received by the SCP.
 *
 ****************************************************************************/
bool CheckResponseMessage ( int A_responseMsgID, InstanceNode* A_node )
{
    MC_STATUS mcStatus;
    bool returnBool = true;

    mcStatus = MC_Get_Value_To_UInt ( A_responseMsgID,
                                      MC_ATT_STATUS,
                                      &A_node->status );
    if ( mcStatus != MC_NORMAL_COMPLETION )
    {
        /* Problem with MC_Get_Value_To_UInt */
        PrintError ( "MC_Get_Value_To_UInt for response status failed", mcStatus );
        strncpy( A_node->statusMeaning, "Unknown Status", sizeof(A_node->statusMeaning) );
        A_node->statusMeaning[sizeof(A_node->statusMeaning)-1] = '\0';

		return false;
    }

    /* MC_Get_Value_To_UInt worked.  Check the response status */

    switch ( A_node->status )
    {
        /* Success! */
        case C_STORE_SUCCESS:
			A_node->storageStatus = DICOMStoragePkg::STORAGE_SUCCEESS;
            strncpy( A_node->statusMeaning, "C-STORE Success.", sizeof(A_node->statusMeaning) );
			::Message(MNOTE, toEndUser | toService | MLoverall, "C-STORE Success.");
            break;
            
        /* Warnings.  Continue execution. */

        case C_STORE_WARNING_ELEMENT_COERCION:
            strncpy( A_node->statusMeaning, "Warning: Element Coersion... Continuing.", sizeof(A_node->statusMeaning) );
            break;

        case C_STORE_WARNING_INVALID_DATASET:
            strncpy( A_node->statusMeaning, "Warning: Invalid Dataset... Continuing.", sizeof(A_node->statusMeaning) );
            break;

        case C_STORE_WARNING_ELEMENTS_DISCARDED:
            strncpy( A_node->statusMeaning, "Warning: Elements Discarded... Continuing.", sizeof(A_node->statusMeaning) ); 
            break;

        /* Errors.  Abort execution. */

        case C_STORE_FAILURE_REFUSED_NO_RESOURCES:
			A_node->storageStatus = DICOMStoragePkg::STORAGE_FAILURE;
            strncpy( A_node->statusMeaning, "ERROR: REFUSED, NO RESOURCES.  ASSOCIATION ABORTING.", sizeof(A_node->statusMeaning) );
            returnBool = false;
            break;

        case C_STORE_FAILURE_INVALID_DATASET:
			A_node->storageStatus = DICOMStoragePkg::STORAGE_FAILURE;
            strncpy( A_node->statusMeaning, "ERROR: INVALID_DATASET.  ASSOCIATION ABORTING.", sizeof(A_node->statusMeaning) );
            returnBool = false;
            break;

        case C_STORE_FAILURE_CANNOT_UNDERSTAND:
			A_node->storageStatus = DICOMStoragePkg::STORAGE_FAILURE;
            strncpy( A_node->statusMeaning, "ERROR: CANNOT UNDERSTAND.  ASSOCIATION ABORTING.", sizeof(A_node->statusMeaning) );
            returnBool = false;
            break;

        case C_STORE_FAILURE_PROCESSING_FAILURE:
			A_node->storageStatus = DICOMStoragePkg::STORAGE_FAILURE;
            strncpy( A_node->statusMeaning, "ERROR: PROCESSING FAILURE.  ASSOCIATION ABORTING.", sizeof(A_node->statusMeaning) );
            returnBool = false;
            break;
            
        default:
            sprintf( A_node->statusMeaning, "Warning: Unknown status (0x%04x)... Continuing.", A_node->status );
            returnBool = false;
            break;
    }

    A_node->statusMeaning[sizeof(A_node->statusMeaning)-1] = '\0';
	::Message(MNOTE, toEndUser | toService | MLoverall, A_node->statusMeaning);

    return returnBool;
}


/****************************************************************************
 *
 *  Function    :   ReadFileFromMedia
 *
 *  Parameters  :   A_options  - Pointer to structure containing input
 *                               parameters to the application
 *                  A_appID    - Application ID registered 
 *                  A_filename - Name of file to open
 *                  A_msgID    - The message ID of the message to be opened
 *                               returned here.
 *                  A_syntax   - The transfer syntax the message was encoded
 *                               in is returned here.
 *                  A_bytesRead- Total number of bytes read in image.  Used
 *                               only for display and to calculate the
 *                               transfer rate.
 *
 *  Returns     :   true on success
 *                  false on failure to read the object
 *
 *  Description :   This function reads a file in the DICOM Part 10 (media)
 *                  file format.  Before returning, it determines the
 *                  transfer syntax the file was encoded as, and converts
 *                  the file into the tool kit's "message" file format
 *                  for use in the network routines.
 *
 ****************************************************************************/
bool ReadFileFromMedia(       STORAGE_OPTIONS&  A_options,
                              int               A_appID,
                              char*             A_filename,
                              int*              A_msgID,
                              TRANSFER_SYNTAX*  A_syntax,
                              size_t*           A_bytesRead )
{
    CBinfo      callbackInfo;
    MC_STATUS   mcStatus;
    char        transferSyntaxUID[UI_LENGTH+2];

    if (A_options.Verbose)
    {
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Reading %s in DICOM Part 10 format", A_filename);
    }

    /*
     * Create new File object 
     */
    mcStatus = MC_Create_Empty_File(A_msgID, A_filename);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("Unable to create file object",mcStatus);
        return( false );
    }


    /*
     * Read the file off of disk
     */
    mcStatus = MC_Open_File(A_appID,
                           *A_msgID,
                            &callbackInfo,
                            MediaToFileObj);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        if (callbackInfo.fp)
            fclose(callbackInfo.fp);
        PrintError("MC_Open_File failed, unable to read file from media", mcStatus);
        MC_Free_File(A_msgID);
        return( false );
    }
    
    if (callbackInfo.fp)
        fclose(callbackInfo.fp);

    *A_bytesRead = callbackInfo.bytesRead;
    
    /*
     * Get the transfer syntax UID from the file to determine if the object
     * is encoded in a compressed transfer syntax.  IE, one of the JPEG or
     * the RLE transfer syntaxes.  If we've specified on the command line
     * that we are supporting encapsulated/compressed transfer syntaxes,
     * go ahead and use the object, if not, reject it and return failure.
     *
     * Note that if encapsulated transfer syntaxes are supported, the 
     * services lists in the mergecom.app file must be expanded using 
     * transfer syntax lists to contain the JPEG syntaxes supported. 
     * Also, the transfer syntaxes negotiated for each service should be 
     * saved (as retrieved by the MC_Get_First/Next_Acceptable service
     * calls) to match with the actual syntax of the object.  If they do
     * not match the encoding of the pixel data may have to be modified
     * before the file is sent over the network.
     */
    mcStatus = MC_Get_Value_To_String(*A_msgID, 
                            MC_ATT_TRANSFER_SYNTAX_UID,
                            sizeof(transferSyntaxUID),
                            transferSyntaxUID);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Get_Value_To_String failed for transfer syntax UID",
                   mcStatus);
        MC_Free_File(A_msgID);
        return false;
    }

    mcStatus = MC_Get_Enum_From_Transfer_Syntax(
                transferSyntaxUID,
                A_syntax);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        ::Message(MWARNING, toEndUser | toService | MLoverall, "Invalid transfer syntax UID contained in the file: %s",
               transferSyntaxUID);
        MC_Free_File(A_msgID);
        return false; 
    } 

    /*
     * If we don't handle encapsulated transfer syntaxes, let's check the
     * image transfer syntax to be sure it is not encoded as an encapsulated
     * transfer syntax.
     */
    if (!A_options.HandleEncapsulated)
    {
        switch (*A_syntax)
        {
            case IMPLICIT_LITTLE_ENDIAN:
            case EXPLICIT_LITTLE_ENDIAN:
            case EXPLICIT_BIG_ENDIAN:
            case IMPLICIT_BIG_ENDIAN:
            case DEFLATED_EXPLICIT_LITTLE_ENDIAN:
                break;
                
            case RLE:
            case JPEG_BASELINE:
            case JPEG_EXTENDED_2_4:
            case JPEG_EXTENDED_3_5:
            case JPEG_SPEC_NON_HIER_6_8:
            case JPEG_SPEC_NON_HIER_7_9:
            case JPEG_FULL_PROG_NON_HIER_10_12:
            case JPEG_FULL_PROG_NON_HIER_11_13:
            case JPEG_LOSSLESS_NON_HIER_14:
            case JPEG_LOSSLESS_NON_HIER_15:
            case JPEG_EXTENDED_HIER_16_18:
            case JPEG_EXTENDED_HIER_17_19:
            case JPEG_SPEC_HIER_20_22:
            case JPEG_SPEC_HIER_21_23:
            case JPEG_FULL_PROG_HIER_24_26:
            case JPEG_FULL_PROG_HIER_25_27:
            case JPEG_LOSSLESS_HIER_28:
            case JPEG_LOSSLESS_HIER_29:
            case JPEG_LOSSLESS_HIER_14:
            case JPEG_2000_LOSSLESS_ONLY:
            case JPEG_2000:
#ifdef linux
			case JPEG_2000_MC_LOSSLESS_ONLY:
            case JPEG_2000_MC:
            case JPEG_LS_LOSSLESS:
            case JPEG_LS_LOSSY:
            case MPEG2_MPML:
#endif
            case PRIVATE_SYNTAX_1:
            case PRIVATE_SYNTAX_2:
                ::Message(MWARNING, toEndUser | toService | MLoverall, "Encapsulated transfer syntax (%s) image specified", 
                       GetSyntaxDescription(*A_syntax));
                ::Message(MWARNING, toEndUser | toService | MLoverall, "         Not sending image.");
                MC_Free_File(A_msgID);
                return false; 
            case INVALID_TRANSFER_SYNTAX:
                ::Message(MWARNING, toEndUser | toService | MLoverall, "Invalid transfer syntax (%s) specified", 
                       GetSyntaxDescription(*A_syntax));
                ::Message(MWARNING, toEndUser | toService | MLoverall, "         Not sending image.");
                MC_Free_File(A_msgID);
                return false; 
        }
    }


    if (A_options.Verbose)
        ::Message(MNOTE, toEndUser | toService | MLoverall, "Reading DICOM Part 10 format file in %s: %s", 
               GetSyntaxDescription(*A_syntax),
               A_filename);
   
    /*
     * Convert the "file" object into a "message" object.  This is done
     * because the MC_Send_Request_Message call requires that the object
     * be a "message" object.  Any of the tags in the message can be
     * accessed when the object is a "file" object.  
     */
    mcStatus = MC_File_To_Message( *A_msgID );
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("Unable to convert file object to message object", mcStatus);
        MC_Free_File(A_msgID);
        return( false );
    }
        
    return true;
    
} /* ReadFileFromMedia() */


/****************************************************************************
 *
 *  Function    :   ReadMessageFromFile
 *
 *  Parameters  :   A_options  - Reference to structure containing input
 *                               parameters to the application
 *                  A_filename - Name of file to open
 *                  A_format   - Enum containing the format of the object
 *                  A_msgID    - The message ID of the message to be opened
 *                               returned here.
 *                  A_syntax   - The transfer syntax read is returned here.
 *                  A_bytesRead- Total number of bytes read in image.  Used
 *                               only for display and to calculate the
 *                               transfer rate.
 *
 *  Returns     :   true  on success
 *                  false on failure to open the file
 *
 *  Description :   This function reads a file in the DICOM "stream" format.
 *                  This format contains no DICOM part 10 header information.
 *                  The transfer syntax of the object is contained in the 
 *                  A_format parameter.  
 *
 *                  When this function returns failure, the caller should
 *                  not do any cleanup, A_msgID will not contain a valid
 *                  message ID.
 *
 ****************************************************************************/
bool ReadMessageFromFile(     STORAGE_OPTIONS&  A_options,
                              char*             A_filename,
                              FORMAT_ENUM       A_format,
                              int*              A_msgID,
                              TRANSFER_SYNTAX*  A_syntax,
                              size_t*           A_bytesRead ) 
{
    MC_STATUS               mcStatus;
    unsigned long           errorTag;
    CBinfo                  callbackInfo;  
    int                     retStatus;
    
    /*
     * Determine the format
     */
    switch( A_format )
    {
        case IMPLICIT_LITTLE_ENDIAN_FORMAT: 
            *A_syntax = IMPLICIT_LITTLE_ENDIAN; 
            if (A_options.Verbose)
                ::Message(MNOTE, toEndUser | toService | MLoverall, "Reading DICOM \"stream\" format file in %s: %s", 
                       GetSyntaxDescription(*A_syntax),
                       A_filename);
            break;
        case IMPLICIT_BIG_ENDIAN_FORMAT:    
            *A_syntax = IMPLICIT_BIG_ENDIAN; 
            if (A_options.Verbose)
                ::Message(MNOTE, toEndUser | toService | MLoverall, "Reading DICOM \"stream\" format file in %s: %s", 
                       GetSyntaxDescription(*A_syntax),
                       A_filename);
            break;
        case EXPLICIT_LITTLE_ENDIAN_FORMAT: 
            *A_syntax = EXPLICIT_LITTLE_ENDIAN; 
            if (A_options.Verbose)
                ::Message(MNOTE, toEndUser | toService | MLoverall, "Reading DICOM \"stream\" format file in %s: %s", 
                       GetSyntaxDescription(*A_syntax),
                       A_filename);
            break;
        case EXPLICIT_BIG_ENDIAN_FORMAT:    
            *A_syntax = EXPLICIT_BIG_ENDIAN; 
            if (A_options.Verbose)
                ::Message(MNOTE, toEndUser | toService | MLoverall, "Reading DICOM \"stream\" format file in %s: %s", 
                       GetSyntaxDescription(*A_syntax),
                       A_filename);
            break;
        default: 
            return false;
    }

    /*
     * Open an empty message object to load the image into
     */
    mcStatus = MC_Open_Empty_Message(A_msgID);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("Unable to open empty message", mcStatus);
        return false;
    }

    /*
     * Open and stream message from file
     */
    callbackInfo.fp = fopen(A_filename, BINARY_READ);

    if (!callbackInfo.fp)
    {
        ::Message(MWARNING, toEndUser | toService | MLoverall, "Unable to open %s.", A_filename);
        MC_Free_Message(A_msgID);
        return false;
    }

    retStatus = setvbuf(callbackInfo.fp, (char *)NULL, _IOFBF, 32768);
    if ( retStatus != 0 )
    {
        ::Message(MWARNING, toEndUser | toService | MLoverall, "Unable to set IO buffering on input file.");
    }
    
    mcStatus = MC_Stream_To_Message(*A_msgID,
                                    MC_ATT_GROUP_0008_LENGTH, 
                                    0xffffFFFF,
                                    *A_syntax,
                                    &errorTag,
                                    (void*) &callbackInfo, /* data for StreamToMsgObj */
                                    StreamToMsgObj);

    if (callbackInfo.fp)
        fclose(callbackInfo.fp);

    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Stream_To_Message error, possible wrong transfer syntax guessed",
            mcStatus);
        MC_Free_Message(A_msgID);
        return false;
    }         

    *A_bytesRead = callbackInfo.bytesRead;

    return true;

} /* ReadMessageFromFile() */


/****************************************************************************
 *
 *  Function    :   MediaToFileObj
 *
 *  Parameters  :   A_fileName   - Filename to open for reading
 *                  A_userInfo   - Pointer to an object used to preserve
 *                                 data between calls to this function.
 *                  A_dataSize   - Number of bytes read
 *                  A_dataBuffer - Pointer to buffer of data read
 *                  A_isFirst    - Set to non-zero value on first call
 *                  A_isLast     - Set to 1 when file has been completely 
 *                                 read
 *
 *  Returns     :   MC_NORMAL_COMPLETION on success
 *                  any other MC_STATUS value on failure.
 *
 *  Description :   Callback function used by MC_Open_File to read a file
 *                  in the DICOM Part 10 (media) format.
 *
 ****************************************************************************/
MC_STATUS NOEXP_FUNC MediaToFileObj( char*     A_filename,
                                 void*     A_userInfo,
                                 int*      A_dataSize,
                                 void**    A_dataBuffer,
                                 int       A_isFirst,
                                 int*      A_isLast)
{
	CBinfo*         callbackInfo = (CBinfo*)A_userInfo;
    size_t          bytes_read;
    int             retStatus;

    if (!A_userInfo)
        return MC_CANNOT_COMPLY;

    if (A_isFirst)
    {
        callbackInfo->bytesRead = 0;
        callbackInfo->fp = fopen(A_filename, BINARY_READ);
        
        retStatus = setvbuf(callbackInfo->fp, (char *)NULL, _IOFBF, 32768);
        if ( retStatus != 0 )
        {
            ::Message(MWARNING, toEndUser | toService | MLoverall, "Unable to set IO buffering on input file.");
        }
    }
    
    if (!callbackInfo->fp)
       return MC_CANNOT_COMPLY;

    bytes_read = fread(callbackInfo->buffer, 1, sizeof(callbackInfo->buffer),
                       callbackInfo->fp);
    if (ferror(callbackInfo->fp))
        return MC_CANNOT_COMPLY;

    if (feof(callbackInfo->fp))
    {
        *A_isLast = 1;
        fclose(callbackInfo->fp);
        callbackInfo->fp = NULL;
    }
    else
        *A_isLast = 0;

    *A_dataBuffer = callbackInfo->buffer;
    *A_dataSize = (int)bytes_read;
    callbackInfo->bytesRead += bytes_read;
    return MC_NORMAL_COMPLETION;
    
} /* MediaToFileObj() */


/*************************************************************************
 *
 *  Function    :  StreamToMsgObj
 *
 *  Parameters  :  A_msgID         - Message ID of message being read
 *                 A_CBinformation - user information passwd to callback
 *                 A_isFirst       - flag to tell if this is the first call
 *                 A_dataSize      - length of data read
 *                 A_dataBuffer    - buffer where read data is stored
 *                 A_isLast        - flag to tell if this is the last call
 *
 *  Returns     :  MC_NORMAL_COMPLETION on success
 *                 any other return value on failure.
 *
 *  Description :  Reads input stream data from a file, and places the data
 *                 into buffer to be used by the MC_Stream_To_Message function.
 *
 **************************************************************************/
MC_STATUS NOEXP_FUNC StreamToMsgObj( int        A_msgID,
                                     void*      A_CBinformation,
                                     int        A_isFirst,
                                     int*       A_dataSize,
                                     void**     A_dataBuffer,
                                     int*       A_isLast)
{
    size_t          bytesRead;
    CBinfo*         callbackInfo = (CBinfo*)A_CBinformation;

    if (A_isFirst)
        callbackInfo->bytesRead = 0L;
        
    bytesRead = fread(callbackInfo->buffer, 1,
                      sizeof(callbackInfo->buffer),
                      callbackInfo->fp);
    if (ferror(callbackInfo->fp))
    {
        perror("\tRead error when streaming message from file.\n");
        return MC_CANNOT_COMPLY;
    }

    if (feof(callbackInfo->fp))
    {
        *A_isLast = 1;
        fclose(callbackInfo->fp);
        callbackInfo->fp = NULL;
    }
    else
        *A_isLast = 0;

    *A_dataBuffer = callbackInfo->buffer;
    *A_dataSize = (int)bytesRead;

    callbackInfo->bytesRead += bytesRead;

    return MC_NORMAL_COMPLETION;
} /* StreamToMsgObj() */


/****************************************************************************
 *
 *  Function    :   CheckValidVR
 *
 *  Parameters  :   A_VR - string to check for valid VR.
 *
 *  Returns     :   bool
 *
 *  Description :   Check to see if this char* is a valid VR.  This function
 *                  is only used by CheckFileFormat.
 *
 ****************************************************************************/
bool CheckValidVR( char    *A_VR)
{
    static const char* const VR_Table[27] = 
    {   
        "AE", "AS", "CS", "DA", "DS", "DT", "IS", "LO", "LT", 
        "PN", "SH", "ST", "TM", "UT", "UI", "SS", "US", "AT", 
        "SL", "UL", "FL", "FD", "UN", "OB", "OW", "OL", "SQ" 
    };
    int i;
   
    for (i =  0; i < 27; i++)
    {
        if ( !strcmp( A_VR, VR_Table[i] ) )
            return true;
    }

    return false;
} /* CheckValidVR() */


/****************************************************************************
 *
 *  Function    :    CheckFileFormat
 *
 *  Parameters  :    Afilename      file name of the image which is being
 *                                  checked for a format.
 *
 *  Returns     :    FORMAT_ENUM    enumberation of possible return values
 *
 *  Description :    Tries to determing the messages transfer syntax.  
 *                   This function is not fool proof!  It is mainly 
 *                   useful for testing puposes, and as an example as
 *                   to how to determine an images format.  This code
 *                   should probably not be used in production equipment,
 *                   unless the format of objects is known ahead of time,
 *                   and it is guarenteed that this algorithm works on those
 *                   objects.
 *
 ****************************************************************************/
FORMAT_ENUM CheckFileFormat( const char*    A_filename )
{
    FILE*            fp;
    char             signature[5] = "\0\0\0\0";
    char             vR[3] = "\0\0";

    union
    {
        unsigned short   groupNumber;
        char      i[2];
    } group;
    
    unsigned short   elementNumber;
    
    union
    {
        unsigned short  shorterValueLength;
        char      i[2];
    } aint;
    
    union
    {
        unsigned long   valueLength;
        char      l[4];
    } along;
   
   

    if ( (fp = fopen(A_filename, BINARY_READ)) != NULL)
    {
        if (fseek(fp, 128, SEEK_SET) == 0)
        {
            /*
             * Read the signature, only 4 bytes
             */
            if (fread(signature, 1, 4, fp) == 4)
            {
                /* 
                 * if it is the signature, return true.  The file is 
                 * definately in the DICOM Part 10 format.
                 */
                if (!strcmp(signature, "DICM"))
                {
                    fclose(fp);
					return MEDIA_FORMAT;
                }
            }
        }
        
        fseek(fp, 0, SEEK_SET);
        
        /*
         * Now try and determine the format if it is not media
         */
        if (fread(&group.groupNumber, 1, sizeof(group.groupNumber), fp) != 
            sizeof(group.groupNumber))
        {
            ::Message(MWARNING, toEndUser | toService | MLoverall, "ERROR: reading Group Number");
            return UNKNOWN_FORMAT;
        }
        if (fread(&elementNumber, 1, sizeof(elementNumber), fp) !=
            sizeof(elementNumber))
        {
            ::Message(MWARNING, toEndUser | toService | MLoverall, "ERROR: reading Element Number");
            return UNKNOWN_FORMAT;
        }

        if (fread(vR, 1, 2, fp) != 2)
        {
            ::Message(MWARNING, toEndUser | toService | MLoverall, "ERROR: reading VR");
            return UNKNOWN_FORMAT;
        }

        /*
         * See if this is a valid VR, if not then this is implicit VR
         */
        if (CheckValidVR(vR))
        {
            /*
             * we know that this is an explicit endian, but we don't
             * know which endian yet.
             */
            if (!strcmp(vR, "OB") 
             || !strcmp(vR, "OW") 
             || !strcmp(vR, "OL") 
             || !strcmp(vR, "UT") 
             || !strcmp(vR, "UN") 
             || !strcmp(vR, "SQ"))
            {
                /* 
                 * need to read the next 2 bytes which should be set to 0
                 */
                if (fread(vR, 1, 2, fp) != 2)
                    ::Message(MWARNING, toEndUser | toService | MLoverall, "ERROR: reading VR");
                else if (vR[0] == '\0' && vR[1] == '\0')
                {
                    /*
                     * the next 32 bits is the length
                     */
                    if (fread(&along.valueLength, 1, sizeof(along.valueLength),
                        fp) != sizeof(along.valueLength))
                        ::Message(MWARNING, toEndUser | toService | MLoverall, "ERROR: reading Value Length");
                    fclose(fp);

                    /*
                     * Make the assumption that if this tag has a value, the
                     * length of the value is going to be small, and thus the
                     * high order 2 bytes will be set to 0.  If the first
                     * bytes read are 0, then this is a big endian syntax.
                     *
                     * If the length of the tag is zero, we look at the 
                     * group number field.  Most DICOM objects start at
                     * group 8. Test for big endian format with the group 8
                     * in the second byte, or else defailt to little endian
                     * because it is more common.
                     */
                    if (along.valueLength)
                    {
                        if ( along.l[0] == '\0' && along.l[1] == '\0') 
							return EXPLICIT_BIG_ENDIAN_FORMAT;
                        return EXPLICIT_LITTLE_ENDIAN_FORMAT;
                    }
                    else
                    {
                        if (group.i[1] == 8)
                            return EXPLICIT_BIG_ENDIAN_FORMAT;
                        return EXPLICIT_LITTLE_ENDIAN_FORMAT;
                    }
                }
                else
                {
                    ::Message(MWARNING, toEndUser | toService | MLoverall, "ERROR: Data Element not correct format");
                    fclose(fp);
                }
            }
            else
            {
                /*
                 * the next 16 bits is the length
                 */
                if (fread(&aint.shorterValueLength, 1,
                    sizeof(aint.shorterValueLength), fp) !=
                    sizeof(aint.shorterValueLength))
                    ::Message(MWARNING, toEndUser | toService | MLoverall, "ERROR: reading short Value Length");
                fclose(fp);

                /*
                 * Again, make the assumption that if this tag has a value,
                 * the length of the value is going to be small, and thus the
                 * high order byte will be set to 0.  If the first byte read
                 * is 0, and it has a length then this is a big endian syntax.
                 * Because there is a chance the first tag may have a length
                 * greater than 16 (forcing both bytes to be non-zero, 
                 * unless we're sure, use the group length to test, and then
                 * default to explicit little endian.
                 */
                if  (aint.shorterValueLength 
                 && (aint.i[0] == '\0'))
					return EXPLICIT_BIG_ENDIAN_FORMAT;
				else
                {
                    if (group.i[1] == 8)
                        return EXPLICIT_BIG_ENDIAN_FORMAT;
                    return EXPLICIT_LITTLE_ENDIAN_FORMAT;
                }
            }
        }
        else
        {
            /* 
             * What we read was not a valid VR, so it must be implicit
             * endian, or maybe format error
             */
            if (fseek(fp, -2L, SEEK_CUR) != 0) 
            {
                ::Message(MWARNING, toEndUser | toService | MLoverall, "ERROR: seeking in file");
                return UNKNOWN_FORMAT;
            }

            /*
             * the next 32 bits is the length
             */
            if (fread(&along.valueLength, 1, sizeof(along.valueLength), fp) !=
                sizeof(along.valueLength))
                ::Message(MWARNING, toEndUser | toService | MLoverall, "ERROR: reading Value Length");
            fclose(fp);

            /*
             * This is a big assumption, if this tag length is a
             * big number, the Endian must be little endian since
             * we assume the length should be small for the first
             * few tags in this message.
             */
            if (along.valueLength)
            {
                if ( along.l[0] == '\0' && along.l[1] == '\0' ) 
                    return IMPLICIT_BIG_ENDIAN_FORMAT;
                return IMPLICIT_LITTLE_ENDIAN_FORMAT;
            }
            else
            {
                if (group.i[1] == 8)
                    return IMPLICIT_BIG_ENDIAN_FORMAT;
                return IMPLICIT_LITTLE_ENDIAN_FORMAT;
            }
        }
    }
    return UNKNOWN_FORMAT;
} /* CheckFileFormat() */


/****************************************************************************
 *
 *  Function    :   Create_Inst_UID
 *
 *  Parameters  :   none
 *                  
 *  Returns     :   A pointer to a new UID
 *
 *  Description :   This function creates a new UID for use within this 
 *                  application.  Note that this is not a valid method
 *                  for creating UIDs within DICOM because the "base UID"
 *                  is not valid.  
 *                  UID Format:
 *                  <baseuid>.<deviceidentifier>.<serial number>.<process id>
 *                       .<current date>.<current time>.<counter>
 *
 ****************************************************************************/
char * Create_Inst_UID()
{
    static short UID_CNTR = 0;
    static char  deviceType[] = "1";
    static char  serial[] = "1";
    static char  Sprnt_uid[68];
    char         creationDate[68];
    char         creationTime[68];
    time_t       timeReturn;
    struct tm*   timePtr;
#ifdef linux
    unsigned long pid = getpid();
#endif


    timeReturn = time(NULL);
    timePtr = localtime(&timeReturn);
    sprintf(creationDate, "%d%d%d",
           (timePtr->tm_year + 1900),
           (timePtr->tm_mon + 1),
            timePtr->tm_mday);
    sprintf(creationTime, "%d%d%d",
            timePtr->tm_hour,
            timePtr->tm_min,
            timePtr->tm_sec);

#ifdef linux    
    sprintf(Sprnt_uid, "2.16.840.1.999999.%s.%s.%ld.%s.%s.%d", 
                       deviceType,
                       serial,
                       pid,
                       creationDate,
                       creationTime,
                       UID_CNTR++);
#else
    sprintf(Sprnt_uid, "2.16.840.1.999999.%s.%s.%s.%s.%d", 
                       deviceType,
                       serial,
                       creationDate,
                       creationTime,
                       UID_CNTR++);
#endif

    return(Sprnt_uid);
}

/****************************************************************************
 *
 *  Function    :   findNode
 *
 *  Description :   Return the index (from 0 up) of a matching SOPClassUID and
 *                  SOPInstanceUID pair. Or return -1 if no matching found.
 *
 ****************************************************************************/
int findNode(DICOMStoragePkg::ResultByFileList& resultByFiles, 
			 char* sopClassUID, 
			 char* sopInstanceUID)
{
    unsigned int i;
    
    /*
     * Go through the file list
     */
    for (i=0; i<resultByFiles.length(); i++)
    {
        if ( !strcmp(resultByFiles[i].SOPClassUID.in(), sopClassUID) &&
			 !strcmp(resultByFiles[i].SOPInstanceUID.in(), sopInstanceUID) )
            return i;
    }

	return -1;
}

void GetCstoreDefaultParameters()
{
  char line[256];
  unsigned int i;

  // Set default values
  strcpy(DefaultTransferSyntax, "IMPLICIT_LITTLE_ENDIAN");
  AsyncCommitIncomingPort = -1;

  std::ifstream cstoreConfigFile("data/Facility/Cstore/cstoredefaults.txt");
  if (!cstoreConfigFile)
    {
		::Message( MWARNING, MLoverall | toService | toDeveloper, "\nCstore: Unable to open \"data/Facility/Cstore/cstoredefaults.txt\" for reading.");
#ifdef DEBUG_PRINTF
		printf("Cstore: Unable to open \"data/Facility/Cstore/cstoredefaults.txt\" for reading.\n");
#endif
	  return;
    }

  i=1;
  memset(line, 0, sizeof(line));
  while(!cstoreConfigFile.getline(line, sizeof(line)).eof())
  {
	  if(strlen(line) > 0)
		  line[strlen(line)] = '\0';
	  else
		  line[0] = '\0';

	  if (isalpha(line[0]) || isdigit(line[0]))
	  {
		  if(i==1)
		  {
			memset(LocalSystemCallingAE, 0, sizeof(LocalSystemCallingAE));
			strncpy(LocalSystemCallingAE, line, sizeof(LocalSystemCallingAE));
			LocalSystemCallingAE[sizeof(LocalSystemCallingAE)-1]='\0';

			::Message( MNOTE, MLoverall | toService | toDeveloper, "Set LocalSystemCallingAETitle = %s", LocalSystemCallingAE);
#ifdef DEBUG_PRINTF
			printf("Set LocalSystemCallingAETitle = %s\n", LocalSystemCallingAE);
#endif
		  }
		  else if(i==2)
		  {
		    AsyncCommitIncomingPort = atoi(line);

			::Message( MNOTE, MLoverall | toService | toDeveloper, "Set Asynchronous_Commit_Incoming_Port = %d", AsyncCommitIncomingPort);
#ifdef DEBUG_PRINTF
			printf("Set Asynchronous_Commit_Incoming_Port = %d\n", AsyncCommitIncomingPort);
#endif
		  }
		  else if(i==3)
		  {
			memset(DefaultTransferSyntax, 0, sizeof(DefaultTransferSyntax));
			strncpy(DefaultTransferSyntax, line, sizeof(DefaultTransferSyntax));
			DefaultTransferSyntax[sizeof(DefaultTransferSyntax)-1]='\0';

			::Message( MNOTE, MLoverall | toService | toDeveloper, "Set DEFAULT_TRANSFER_SYNTAX = %s", DefaultTransferSyntax);
#ifdef DEBUG_PRINTF
			printf("Set DEFAULT_TRANSFER_SYNTAX = %s\n", DefaultTransferSyntax);
#endif
			break;
		  }

		  i++;
	  }
      memset(line, 0, sizeof(line));
  }
}
