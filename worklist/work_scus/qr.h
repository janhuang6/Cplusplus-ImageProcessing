/*************************************************************************
 *
 *       System: MergeCOM-3 Query/Retrieve Sample Applicatin Include File
 *
 *    $Workfile: qr.h $
 *
 *    $Revision: 1.1 $
 *
 *  Description: This is the include file for the Q/R SCU & SCP samples.
 *
 ************************************************************************/
/* $NoKeywords: $ */

#ifndef MTI_QR_H
#define MTI_QR_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * File read and write mode defines for various platforms
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


#define FALSE                   0
#define TRUE                    1
#define FAILURE                 0
#define SUCCESS                 1
#define NOTFOUND                0
#define FOUND                   1

        
/* 
 * attribute lengths for various text VR's.  Add 1 for null character.
 */
#define PN_LEN (64+1)
#define LO_LEN (64+1)
#define DA_LEN (10+1)
#define TM_LEN (16+1)
#define SH_LEN (16+1)
#define UI_LEN (64+1)
#define CS_LEN (16+1)
#define IS_LEN (12+1)
#define AE_LEN (16+1)
#define DS_LEN (16+1)


/*
 * Information about our "database" (test.dat)
 */
#define DATABASE_ENTRIES 10
#define USER_INPUT_LEN 128
#define INIT_ARRAY(array, size, init_val) {int ii=0; for(ii=0; ii<size; ii++) array[ii] = init_val;}

/* 
 * "database" read format string 
 */
#define FMT_STR \
"%[^:]%*c%[^:]%*c%[^:]%*c%[^:]%*c%[^:]%*c%[^:]%*c%[^:]%*c%[^:]%*c%[^:]%*c%[^\n]%*c"

/*
** "database" read format string for Modality Worklist
*/
#define WORK_FMT_STR \
"%[^:]%*c%[^:]%*c%[^:]%*c%[^:]%*c%[^:]%*c%[^:]%*c%[^:]%*c%[^:]%*c%[^:]%*c%[^:]%*c%[^\n]%*c"

/* 
 * Search fields.  These are indexes into an array that contains our
 * "database" information.  They are used for easy access of these fields. 
 */
#define PID     0       /* Patient ID */
#define PN      1       /* Patient Name */
#define PBD     2       /* Patient Birthday */
#define PS      3       /* Patient Sex */
#define SI      4       /* Study Instance */
#define SD      5       /* Study Date */
#define ST      6       /* Study Time */
#define SAN     7       /* Study Accession Number */
#define SID     8       /* Study ID */
#define SDI     9       /* Study Description */


/* Search fields for the Modality Worklist sample app */
#define PSS     0       /* scheduled Procedure Step Sequence */
#define SAE     1       /* scheduled Station AE */
#define PSD     2       /* scheduled Procedure Step start Date */
#define PST     3       /* scheduled Procedure Step start Time */
#define MOD     4       /* Modality */
#define PPN     5       /* scheduled Performing Physician's Name */
#define PSI     6       /* scheduled Procedure Step Id */
#define RID     7       /* Requested procedure ID */
#define SID     8       /* Study Instance uID */
#define TPN     9       /* The Patient's Name */
#define TID     10      /* The patient's ID */

#define SERVICELIST_LEN   64    /* List of services supported */
#define FILENAME_LEN      256   /* Length of file name */
#define SERVICENAME_LEN   32    /* Length of a servics name */

/* 
 * Macros 
 */
#define DO_FOREVER              for(;;)
#define TAG(g,c)                (unsigned long)((g<<16)+c)
#define GROUP_VALUE(tag)        (unsigned short)(tag & 0x0000ffff)
#define ELEMENT_VALUE(tag)      (unsigned short)((tag >> 16) & 0x0000ffff)

#define PATIENT_MODEL             "PATIENT_ROOT"
#define STUDY_MODEL               "STUDY_ROOT"
#define PATIENT_STUDY_ONLY_MODEL  "PATIENT_STUDY_ONLY"

#define PATIENT_LEVEL   "PATIENT"
#define STUDY_LEVEL     "STUDY"
#define SERIES_LEVEL    "SERIES"
#define IMAGE_LEVEL     "IMAGE"

/* 
 * DEBUG and VALIDATION defines.  These should be 
 * uncommented to either list messages to file or 
 * validate messages.
 */
/*
#define LIST_MESSAGES
#define VALIDATE_MESSAGES
*/

/* Error levels, passed to the log_error function */
#define ERROR_LVL_1 1
#define ERROR_LVL_2 2
#define ERROR_LVL_3 3

/*
 * Enum for return values within the application
 */   
typedef enum 
{
    QR_FAILURE = 0,
    QR_SUCCESS,
    QR_OFFLINE,
    QR_NOTAG
} QR_STATUS;


/*
 * Data structures to maintain information at each of
 * the query levels.
 */    
typedef struct PATIENT_ROOT_PATIENT_LEVEL
{                                        
    struct PATIENT_ROOT_PATIENT_LEVEL *next;
    char patient_name[PN_LEN+1];
    char patient_id[LO_LEN+1];
    char retrieveAETitle[AE_LEN+1];
    char med_file_id[SH_LEN+1];
    char med_file_uid[UI_LEN+1];
} PRPL;               

typedef struct PATIENT_ROOT_STUDY_LEVEL
{
    struct PATIENT_ROOT_STUDY_LEVEL *next;
    char  study_date[DA_LEN+1];
    char  study_time[TM_LEN+1];
    char  accession_num[SH_LEN+1];       
    char  patient_name[PN_LEN+1];
    char  patient_id[LO_LEN+1];
    char  study_id[SH_LEN+1];
    char  study_inst_uid[UI_LEN+1];      
    char  retrieveAETitle[AE_LEN+1];
    char med_file_id[SH_LEN+1];
    char med_file_uid[UI_LEN+1];
} PRSTL;                     

typedef struct PATIENT_ROOT_SERIES_LEVEL
{
    struct PATIENT_ROOT_SERIES_LEVEL *next;
    char modality[CS_LEN+1];
    char series_num[IS_LEN+1];
    char series_inst_uid[UI_LEN+1];
    char retrieveAETitle[AE_LEN+1];
    char med_file_id[SH_LEN+1];
    char med_file_uid[UI_LEN+1];
} PRSL;                     

typedef struct PATIENT_ROOT_IMAGE_LEVEL
{
    struct PATIENT_ROOT_IMAGE_LEVEL *next;
    char image_num[IS_LEN+1];
    char sop_inst_uid[UI_LEN+1];
    char retrieveAETitle[AE_LEN+1];
    char med_file_id[SH_LEN+1];
    char med_file_uid[UI_LEN+1];
} PRIL;

/*
** The strcture of a node of a linked list.
*/
typedef struct _NODE
{
    struct _NODE  *next_ptr;
    struct _NODE  *prev_ptr;
    void          *data_ptr;
} NODE;

/*
** The strcture of a head of a linked list.
*/
typedef struct _LIST
{
    NODE  *node_ptr;
    int   num_elements;
    int   node_size;
    NODE  *head_ptr;
} LIST;

/* 
 * Function Prototypes: defined in qr_util.c 
 */
extern void      PrintErrorMessage      ( char* A_prefix, char* A_function,
                                          int A_status, char* A_errorMessage );
extern void      WriteToListFile        ( int A_messageID, char* A_outFileName );
extern void      ValMessage             ( int A_messageID, char* A_name);
extern int       LLCreate               ( LIST *list_ptr, int node_size );
extern int       LLInsert               ( LIST *list_ptr, void *data );
extern void      LLRewind               ( LIST *list_ptr );
extern NODE      *LLPopNode             ( LIST *list_ptr, void *data );
extern NODE      *LLPopNodeN            ( LIST *list_ptr, void *data, int n );
extern int       LLNodes                ( LIST *list_ptr );
extern void      LLLast                 ( LIST *list_ptr );
extern int       LLUpdate               ( LIST *list_ptr, void *new_node );
extern void      *LLGetDataPtr          ( NODE *node_ptr );
extern void      LLDestroy              ( LIST *list_ptr );
extern void      LogError               ( int error_level, char *function_name,
                                          int line_number, MC_STATUS status,
                                          char *format, ... );
extern void      GetCurrentDate         ( char *A_date, char *A_time );
extern char      *Create_Inst_UID       ( void );



#ifdef __cplusplus
}
#endif

#endif /* End MTI_QR_H */
