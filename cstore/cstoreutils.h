#ifndef _CSTOREUTILS_H_
#define _CSTOREUTILS_H_

/*
 * file:	cstoreutils.h
 * purpose:	cstore utility library
 * created:	24-Aug-07
 * property of:	Philips Nuclear Medicine
 *
 * revision history:
 *   24-Aug-07	Jiantao Huang		    initial version
 */

#include <iostream>
#include <list>
#include <vector>
using namespace std;

#include <string.h>

#include "mc3media.h"
#include "mergecom.h" 
#include "mc3msg.h"
#include "mcstatus.h"
#include "diction.h"

#include "acquire/mesg.h"
#include "acquire/threadmutex.h"
#include "control/DICOMStorage_s.h"

#include "echoSCP.h"

//#define DEBUG_PRINTF

/*
 * Module constants
 */

#if defined(_MSDOS)     || defined(__OS2__)   || defined(_WIN32) || \
    defined(_MACINTOSH) || defined(INTEL_WCC) || defined(_RMX3)
#define BINARY_READ "rb"
#define BINARY_WRITE "wb"
#define BINARY_APPEND "rb+"
#define BINARY_READ_APPEND "a+b"
#define BINARY_CREATE "w+b"
#ifdef _MSDOS
#define TEXT_READ "rt"
#define TEXT_WRITE "wt"
#else
#define TEXT_READ "r"
#define TEXT_WRITE "w"
#endif
#else
#define BINARY_READ "r"
#define BINARY_WRITE "w"
#define BINARY_APPEND "r+"
#define BINARY_READ_APPEND "a+"
#define BINARY_CREATE "w+"
#define TEXT_READ "r"
#define TEXT_WRITE "w"
#endif 

/* DICOM VR Lengths */
#define AE_LENGTH 16
#define UI_LENGTH 64

/*
 * Structure to maintain list of instances sent & to be sent.
 * The structure keeps track of all instances and is used
 * in a linked list.
 */
typedef struct instance_node
{
    int    msgID;                       /* messageID of for this node */
    char   fname[1024];                 /* Name of file */
    TRANSFER_SYNTAX transferSyntax;     /* Transfer syntax of file */
    
    char   SOPClassUID[UI_LENGTH+2];    /* SOP Class UID of the file */
    char   serviceName[48];             /* MergeCOM-3 service name for SOP Class */
    char   SOPInstanceUID[UI_LENGTH+2]; /* SOP Instance UID of the file */
    
    size_t imageBytes;                  /* size in bytes of the file */
    
    unsigned int dicomMsgID;            /* DICOM Message ID in group 0x0000 elements */
    unsigned int status;                /* DICOM status value returned for this file. */
    char   statusMeaning[100];          /* Textual meaning of "status" */   
    bool   responseReceived;            /* Bool indicating we've received a response for a sent file */
    bool   failedResponse;              /* Bool saying if a failure response message was received */
    bool   imageSent;                   /* Bool saying if the image has been sent over the association yet */
    bool   mediaFormat;                 /* Bool saying if the image was originally in media format (Part 10) */
    DICOMStoragePkg::StorageStatus storageStatus;  /* Storage result */
    DICOMStoragePkg::CommitStatus commitStatus;  /* Storage commitment result */

    struct instance_node* Next;         /* Pointer to next node in list */

} InstanceNode;


/*
 * class to pass info into Storage class
 */
class StorageData
{	list<string> _filenames;

	bool addFileToList(char* A_fname);
	void freeInstanceList();

public:
	InstanceNode* _instanceList;

	StorageData();
	~StorageData();
	StorageData(const StorageData& obj);
	StorageData& operator=(const StorageData& obj);

	void clear();
	void createLinkedList(const list<string>& filelist);
	bool isEmpty();
};

/*
 * Module type declarations
 */

/*
 * CBinfo is used to callback functions when reading in stream objects
 * and Part 10 format objects.
 */
typedef struct CALLBACKINFO
{
    FILE*         fp;
    /* 
     * Note!   The size of this buffer impacts toolkit performance.  Higher 
     * values in general should result in increased performance of reading
     * files
     */
    char          buffer[64*1024];  
    size_t        bytesRead;
} CBinfo;


/*
 * Used to identify the format of an object
 */
typedef enum
{
    UNKNOWN_FORMAT = 0,
    MEDIA_FORMAT = 1,
    IMPLICIT_LITTLE_ENDIAN_FORMAT,
    IMPLICIT_BIG_ENDIAN_FORMAT,
    EXPLICIT_LITTLE_ENDIAN_FORMAT,
    EXPLICIT_BIG_ENDIAN_FORMAT
} FORMAT_ENUM;

/*
 * HandleNEventAssociation return status
 */
typedef enum
{
    FAILURE = 0,
    SUCCESS = 1,
    TIMEOUT = 2
} NEVENT_ENUM;

/*
 * Structure of local application information for storage 
 */
typedef struct stor_scu_options
{
    char    RemoteAE[AE_LENGTH+2]; 
    char    RemoteHostname[100]; 
    int     RemotePort;
    char    ServiceList[100]; 

    bool    Verbose;
    bool    HandleEncapsulated;
    
    AssocInfo asscInfo;
} STORAGE_OPTIONS;

/*
 * Structure of local application information for commit 
 */
typedef struct commit_scu_options
{
    char    RemoteAE[AE_LENGTH+2]; 
    char    RemoteHostname[128]; 
    int     RemotePort;
	int     RoleReversalWaitTime;
    bool    Verbose;
} COMMIT_OPTIONS;

/*
 * Structure of arguments to pass into StoreFiles() 
 */
class STORE_ARGS
{
public:
	int              applicationID;
	STORAGE_OPTIONS  options;
	StorageData*     storageData;

	~STORE_ARGS();
};

/*
 * Structure of arguments to pass into A/SynchStorageCommitment() 
 */
class COMMIT_ARGS
{
public:
	COMMIT_OPTIONS         options;
    int                    appID;
    DICOMStoragePkg::ResultByStorageTarget* result;

	COMMIT_ARGS();
	COMMIT_ARGS(const COMMIT_ARGS& obj);
	COMMIT_ARGS& operator=(const COMMIT_ARGS& obj);
	~COMMIT_ARGS();
};

/*
** This method PerformMergeInitialization should only be
** called once per application. Not per class instantiation.
** Otherwise, get error MC_LIBRARY_ALREADY_INITIALIZED
*/
int PerformMergeInitialization( const char *mergeIniFile, int *p_applicationID, const char *p_localAppTitle );

void* StoreFiles(void*                         store_args);
bool UpdateNode( InstanceNode*                 A_node );
int GetNumNodes( InstanceNode*                 A_list);
int GetNumOutstandingRequests(InstanceNode*    A_list);

void* SynchStorageCommitment(void*             commit_args);

void* AsynchStorageCommitment(void*            commit_args);

/*
 * EitherStorageCommitment performs synchronous storage commitment first.
 * If timeout, automatically convert into asynchronous storage commitment.
 */
void* EitherStorageCommitment(void*            commit_args);

bool SetAndSendNActionMessage(
                        COMMIT_OPTIONS&                         A_options,
                        int                                     A_associationID,
                        DICOMStoragePkg::ResultByStorageTarget* A_result);

NEVENT_ENUM HandleNEventAssociation(
                        COMMIT_OPTIONS&                         A_options,
                        int                                     A_associationID,
                        DICOMStoragePkg::ResultByStorageTarget* A_result,
						DICOMStoragePkg::CommitType			    A_commitType);
                        
bool ProcessNEventMessage(
                        int                                     A_messageID,
                        DICOMStoragePkg::ResultByStorageTarget* A_result);

bool ReadImage(         STORAGE_OPTIONS&    A_options,
                        int                 A_appID, 
                        InstanceNode*       A_node);
                        
bool SendImage(         STORAGE_OPTIONS&    A_options,
                        int                 A_associationID, 
                        InstanceNode*       A_node);

bool ReadResponseMessages(STORAGE_OPTIONS&  A_options,
                        int                 A_associationID, 
                        int                 A_timeout,
                        InstanceNode**      A_list);

bool CheckResponseMessage ( 
                        int                 A_responseMsgID, 
                        InstanceNode*       A_node );

bool ReadFileFromMedia( STORAGE_OPTIONS&    A_options,
                        int                 A_appID,
                        char*               A_filename,
                        int*                A_msgID,
                        TRANSFER_SYNTAX*    A_syntax,
                        size_t*             A_bytesRead);

bool ReadMessageFromFile( 
                        STORAGE_OPTIONS&    A_options,
                        char*               A_fileName,
                        FORMAT_ENUM         A_format,
                        int*                A_msgID,
                        TRANSFER_SYNTAX*    A_syntax,
                        size_t*             A_bytesRead);

MC_STATUS NOEXP_FUNC MediaToFileObj( 
                        char*               Afilename,
                        void*               AuserInfo,
                        int*                AdataSize,
                        void**              AdataBuffer,
                        int                 AisFirst,
                        int*                AisLast);
                                 
MC_STATUS NOEXP_FUNC StreamToMsgObj( 
                        int                 AmsgID,
                        void*               AcBinformation,
                        int                 AfirstCall,
                        int*                AdataLen,
                        void**              AdataBuffer,
                        int*                AisLast);
                                 
bool CheckValidVR( char    *A_VR);

FORMAT_ENUM CheckFileFormat(const char*           A_filename );
                        
char* Create_Inst_UID();

int findNode(DICOMStoragePkg::ResultByFileList& resultByFiles, 
			 char* sopClassUID, char* sopInstanceUID);

void GetCstoreDefaultParameters();

#endif
