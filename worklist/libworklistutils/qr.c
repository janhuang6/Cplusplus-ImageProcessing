
/*************************************************************************
 *
 *       System: MergeCOM-3 Q/R Sample Application Utility functions
 *
 *    $Workfile: qr_util.c $
 *
 *    $Revision: 1.5 $
 *
 *  Description: 
 *               This file is comprised of utility functions used by
 *               both the SCU and SCP Query/Retrieve Sample Applications.
 *
 ************************************************************************/

/*
 * file:	Copied from qr.c and modified
 * purpose:	for use by worklistutils.
 *
 * inspection history:
 *
 * revision history:
 *   Jiantao Huang		initial version
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if !defined(OS9_68K) && !defined(OS9_PPC)
#include <fcntl.h>
#endif

#ifdef _MSDOS
#include <time.h>
#endif

#include <stdarg.h>

#include "mergecom.h"
#include "mcstatus.h"
#include "qr.h"


/*****************************************************************************
**
** NAME
**    WriteToListFile - Writes a list file 
**
** ARGUMENTS
**    A_messageID      int       Message ID
**    A_outFileName    char *    File name to write list to
**
** DESCRIPTION
**    This function lists the contents of a DICOM message to a specified 
**    output file.  The output is a nice text display of the header 
**    information.
**
** RETURNS
**    Nothing
**
** SEE ALSO
**
*****************************************************************************/
void WriteToListFile ( int A_messageID, const char* A_outFileName )
{
#ifdef LIST_MESSAGES

#if !defined(_WIN32)
    FILE *outfile;
    
    outfile = fopen ( A_outFileName, TEXT_WRITE );
    if ( NULL == outfile )
       ::Message ( MWARNING, MLoverall | toService | toDeveloper, "WriteToListFile: fopen failed for list file %d \n", A_outFileName );
    else
       MC_List_Message ( A_messageID, outfile );

    fclose ( outfile );
#else
    MC_List_Message ( A_messageID, A_outFileName );
#endif

#endif
    return;
} /* end of WriteToListFile */


/*****************************************************************************
**
** NAME
**    ValMessage - Validate a message and print out information
**
** ARGUMENTS
**    A_messageID       int       Message to validate
**    A_name            char *    Description of message to validate
**
** DESCRIPTION
**    This routine validates a message and prints out all of the errors
**    found within the message.
**
** RETURNS
**    None
**
** SEE ALSO
**
*****************************************************************************/
void ValMessage(int A_messageID, const char* A_name)
{
#ifdef VALIDATE_MESSAGES
    MC_STATUS           status;             /* return value from toolkit calls */
    VAL_ERR*            errInfo;            /* Information about errors found in message */
    char                typeStr[4];         /* string for type of message */

    status = MC_Validate_Message(A_messageID,&errInfo,Validation_Level2);

    if (status == MC_MESSAGE_VALIDATES)
    {
        ::Message( MWARNING, MLoverall | toService | toDeveloper, "%s message validated!\n", A_name); 
    }
    else if (status == MC_DOES_NOT_VALIDATE)
    {
        ::Message( MWARNING, MLoverall | toService | toDeveloper, "\n***\tVALIDATION ERROR: %s\n", A_name );
        ::Message( MWARNING, MLoverall | toService | toDeveloper, "(Grp ,Elmt)  Value# Type  Message\n");
        do
        {
            switch (errInfo->Status)
            {
                case MC_NO_CONDITION_FUNCTION:
                case MC_UNABLE_TO_CHECK_CONDITION:
                        strcpy(typeStr,"I");
                        break;
                case MC_NOT_ONE_OF_DEFINED_TERMS:
                case MC_NON_SERVICE_ATTRIBUTE: 
                        strcpy(typeStr,"W");
                        break;
                default:
                        strcpy(typeStr,"E");
                        break; 
            } 
            ::Message( MWARNING, MLoverall | toService | toDeveloper, "(%.4X,%.4X)  %.4d   %s     %s\n",
                      GROUP_VALUE(errInfo->Tag), 
                      ELEMENT_VALUE(errInfo->Tag), 
                      errInfo->ValueNumber,
                                  typeStr,
                      MC_Error_Message(errInfo->Status)); 
                            
            status = MC_Get_Next_Validate_Error(A_messageID,&errInfo);
        } while (status == MC_NORMAL_COMPLETION);

        ::Message( MWARNING, MLoverall | toService | toDeveloper, "\n" );
    } 
    else
    {
        ::Message( MWARNING, MLoverall | toService | toDeveloper, "ValMessage: MC_Validate_Message failed with status = %d \n", status);
    }   
#endif
    return; 
} /* ValMessages */

#ifndef VXWORKS
/*****************************************************************************
**
** NAME
**    GetCurrentDate - Gets today's date in the form of YYYYMMDD,
**                     and the current time in the form of HHMMSS.
**
** SYNOPSIS
**    void GetCurrentDate ( char *A_date, char *A_time )
**
** ARGUMENTS
**    none
**
** DESCRIPTION
**    Obtains todays date, formatted such that it is consistantly eight
**    characters in length where:
**    YYYY = The four digit year
**    MM   = The two digit month
**    DD   = The two digit day of the month
**    and the current time is forrmatted 6 characters in length where:
**    HH   = The two digit hour (0-23)
**    MM   = The two digit minute (0-59)
**    SS   = The two digit second (0-59) <- may also include leap seconds
**                                          depending on the host OS
**    
**    This function fills in the users string, passed in through A_date,
**    with the correctly formatted date string, and the time is filled in
**    via A_time.
**
**    This function WILL produce incorrect results in the year 10000.
**
** SEE ALSO
**
*****************************************************************************/
void
GetCurrentDate( 
                  char *A_date,
                  char *A_time
              )
{
    time_t    the_time;              /* Epoch time                          */
    struct tm *tm_time;              /* Pointer to a struct tm structure    */
    char      year[5];               /* A four digit year string + NULL     */
    char      month[5];              /* A two digit month string + NULL     */
    char      day[5];                /* A two digit day string + NULL       */

    /*
    ** We obtain the current time, so that we know what today's date is.
    */
    time( &the_time );                /* Get time since epoch */
    tm_time = localtime( &the_time ); /* Convert this into a tm structure */
    year[4] = '\0';                   /* NULL terminate our strings  */
    month[2] = '\0';
    day[2] = '\0';
    A_date[8] = '\0';

    /*
    ** Keep in mind that tm_year is the years since 1900.  If we don't want
    ** to have Y2K problems, and hoping that our OS is Y2K complient, we
    ** add the number of years since 1900 to the value '1900' to get the
    ** current year.  IE, 1900 + 100 should be 2000!
    */
    sprintf( year, "%04d", tm_time->tm_year + 1900 );

    /*
    ** tm_mon is in the range of 0-11, so we must add 1 to get the 'human'
    ** form that we're looking for.
    */
    sprintf( month, "%02d", tm_time->tm_mon + 1 );

    /*
    ** tm_mday needs no 'human' conversion...
    */
    sprintf( day, "%02d", tm_time->tm_mday );

    /*
    ** Finally, build our needed strings.
    */
    sprintf( A_date, "%s%s%s", year, month, day );
    sprintf( A_time, "%02d%02d%02d", tm_time->tm_hour,
             tm_time->tm_min, tm_time->tm_sec );
}
#endif /* ifndef VXWORKS */

/* The following functions are used to implement a primitive linked list */

/*****************************************************************************
**
** NAME
**    LLCreate - This function will create an empty linked list structure.
**
** ARGUMENTS
**    list_ptr          LIST *    A pointer to a structure of type LIST.
**    node_size         int       The size of each node that the list
**                                contains.
**
** DESCRIPTION
**    This function will create an empty linked list structure, such that
**    list_ptr's internal structure will point to an empty node, and the
**    number of elements on the list will be set to zero.
**    We want the head list structure to have been already declared.
**    Hence, we take a pointer to it as an argument.
**
** RETURNS
**    MERGE_SUCCESS if the function finishes properly.
**    MERGE_FAILURE if the function detects an error.
**
*****************************************************************************/
int
LLCreate(
          LIST *list_ptr,
          int  node_size
        )
        
{
    NODE *the_node;

    /*
    ** Make the number of elements in the head node 0.
    */
    list_ptr->num_elements = 0;
    
    /*
    ** Setup the size of each node's data element
    */
    list_ptr->node_size = node_size;
    
    /*
    ** Make the pointer to the next node within the head node
    ** point to a newly created node.
    */
    the_node = (NODE *)malloc( sizeof( NODE ) );
    if ( the_node == NULL )
    {
        return( MERGE_FAILURE );
    }
    list_ptr->node_ptr = the_node;
    list_ptr->head_ptr = the_node;
    
    /*
    ** Now that we have a properly setup head node, we will make the
    ** actual data node (the newly created, empty one), point to
    ** nothing.
    */
    list_ptr->node_ptr->prev_ptr = NULL;
    list_ptr->node_ptr->next_ptr = NULL;
    list_ptr->node_ptr->data_ptr = NULL;
    
    return( MERGE_SUCCESS );

}


/*****************************************************************************
**
** NAME
**    LLInsert - This function will insert data onto a linked list.
**
** ARGUMENTS
**    list_ptr          LIST *    A pointer to a structure of type LIST.
**    data              void *    A pointer to the data that will be inserted
**                                in the list.
**
** DESCRIPTION
**    This function will copy the data pointed to by data, onto the linked
**    list that is specified by list_ptr.  The data is copied to the linked
**    list and the data that is passed into this function is NOT disturbed
**    in any way.
**    The next empty node in the list is then created, and pointed to.
**
** RETURNS
**    MERGE_SUCCESS if the function finishes properly.
**    MERGE_FAILURE if the function detects an error.
**
*****************************************************************************/
int
LLInsert(
          LIST *list_ptr,
          void *data
        )
{
    NODE *new_node;
    NODE *old_node;
    
    /*
    ** Save off the current node position, so that it can be reused, later.
    */
    old_node = list_ptr->node_ptr;
    
    /*
    ** malloc space for the data of the node, and make the node's
    ** data pointer point to it.
    */
    old_node->data_ptr = malloc( (size_t)list_ptr->node_size );
    if ( old_node->data_ptr == NULL )
    {
        /*
        ** Should we fail in our attempts to copy the data onto the node,
        ** we must tell our caller that we've failed.
        */
        return( MERGE_FAILURE );
    }
    
    /*
    ** Copy the data from the source to the destination.
    */
    memcpy( old_node->data_ptr, data, (size_t)list_ptr->node_size );
    
    /*
    ** Malloc a new node, making the original node's next_ptr
    ** point to it.
    */
    new_node = (NODE *)malloc( sizeof( NODE ) );
    if ( new_node == NULL )
    {
        /*
        ** Again, if we fail in the attempt to create the next, empty, node
        ** of the list, we must completely fail.
        */
        return( MERGE_FAILURE );
    }
    old_node->next_ptr = new_node;
    
    /*
    ** Here we adjust the head node, making it point to the new, empty
    ** node.  We also increment the number of nodes on the list.
    */
    list_ptr->node_ptr = new_node;    
    list_ptr->num_elements++;
    
    /*
    ** Set up the new node by making its previous pointer point to the 
    ** original node, its next pointer point to NULL, and its data pointer
    ** point to NULL
    */
    new_node->prev_ptr = old_node;
    new_node->next_ptr = NULL;
    new_node->data_ptr = NULL;
    
    /*
    ** Only after we have been able to complete all of these operations may
    ** we return a successful status.
    */
    return( MERGE_SUCCESS );
    
}


/*****************************************************************************
**
** NAME
**    LLRewind - This function will rewind a list.
**
** ARGUMENTS
**    list_ptr          LIST *    A pointer to a structure of type LIST.
**
** DESCRIPTION
**    This function will take a list that already contains data, and make
**    the head node of the list point back to the first node on the list.
**    It is assumed that this function will be used as the first step in
**    obtaining the information stored in a list.
**
** RETURNS
**   Nothing
**
** LIMITATIONS
**   Calling LLRewind and then using LLInsert to place new objects on the
**   list will cause the list's original contents to be lost and the used
**   memory to be lost as well.  Since these are simple list functions, and
**   are not intended to be a complete implementation of a general list
**   library, there is currently no method of stopping a user from doing
**   this.
*****************************************************************************/
void
LLRewind(
          LIST *list_ptr
        )
{
    /*
    ** This is pretty simple,  since the head structure of the list
    ** already contains a pointer to the first node in a list, we
    ** make the node pointer in the list point to the head of the list.
    */
    list_ptr->node_ptr = list_ptr->head_ptr;
}


/*****************************************************************************
**
** NAME
**    LLPopNode - This function will pop the current item off of a list.
**
** ARGUMENTS
**    list_ptr          LIST *    A pointer to a structure of type LIST.
**    data              void *    A pointer to the data to obtain.
**
** DESCRIPTION
**    This function will take a list that already contains some data, 
**    and copy this data to the place pointed to by "data", which is
**    passed into this function.
**
** RETURNS
**    A pointer to the node which contains the data desired.
**
*****************************************************************************/
NODE *
LLPopNode(
           LIST *list_ptr,
           void *data
         )
{
    NODE *current_node;

    current_node = list_ptr->node_ptr;

    /*
    ** Should someone call this function without having rewound the list,
    ** (I.E., we are at the end of the list), we will just return NULL.
    */
    if ( list_ptr->node_ptr->data_ptr == NULL )
    {
        data = NULL;
        return( NULL );
    }
    
    /*
    ** Get the data
    */
    memcpy( data, list_ptr->node_ptr->data_ptr, (size_t)list_ptr->node_size );

    /*
    ** Make us look at the next node.
    */
    list_ptr->node_ptr = list_ptr->node_ptr->next_ptr;
    
    return( current_node );    
}


/*****************************************************************************
**
** NAME
**    LLPopNodeN - This function will pop a specific item off of a list.
**
** ARGUMENTS
**    list_ptr          LIST *    A pointer to a structure of type LIST.
**    data              void *    A pointer to the node that contains the
**                                data.
**    n                 int       A node to return in data
**
** DESCRIPTION
**    This function will take a list that already contains some data, 
**    rewind the list, and then walk the list until the node that is
**    numerically found in the n'th position is identified.  After
**    the n'th item is found, the data contained within that node is
**    copied to the place pointed to by data, and a pointer to this node
**    is returned.
**
** RETURNS
**    A pointer to the node which contains the data desired.
**    The data from the n'th node.
**
** LIMITATION
**    This function leaves the head pointer of the list, pointing to the
**    item it has just retrieved.  Thus, a while loop calling LLPopNodeN
**    will NOT increment the head node's current pointer!  This will result
**    in an infinite loop.  If you want to walk the list in sequential
**    order, use LLPopNode.  If you want to walk the list in a user defined
**    order, use this function and define the N'th node yourself.
**
*****************************************************************************/
NODE *
LLPopNodeN(
           LIST *list_ptr,
           void *data,
           int  n
         )
{
    NODE *current_node;
    int  count = 1;

    current_node = list_ptr->node_ptr;

    /*
    ** Should someone call this function without having rewound the list,
    ** (I.E., we are at the end of the list), we will rewind it for them.
    */
    LLRewind( list_ptr );
    
    while ( count <= n )
    {
        /*
        ** Get the data
        */
        current_node = LLPopNode( list_ptr, data );
        count++;
    }
    
    /*
    ** Make sure that the head node's current pointer is pointing at the
    ** node we've just retrieved.
    */
    list_ptr->node_ptr = current_node;

    return( current_node );    
}


/*****************************************************************************
**
** NAME
**    LLNodes - This function will return the number of nodes in the list
**
** ARGUMENTS
**    list_ptr          LIST *    A pointer to a structure of type LIST.
**
** DESCRIPTION
**    This function returns the number of nodes in the list.  Usage of this
**    function is desireable, thus examining the structres of the head list
**    object directly should be avoided.
**
** RETURNS
**   The number of nodes in the list
**
*****************************************************************************/
int
LLNodes(
        LIST *list_ptr
      )
{
    return( list_ptr->num_elements );
}


/*****************************************************************************
**
** NAME
**    LLLast - This function will make current, the last item on the list.
**
** ARGUMENTS
**    list_ptr          LIST *    A pointer to a structure of type LIST.
**
** DESCRIPTION
**    This function will take a list that already contains data, and make
**    the head node of the list point back to the last node on the list.
**    The last item on any list should NOT contain any data.  That is,
**    after calling LLLast, we are then able to continue inserting items
**    on a list just a though we were doing it in sequence.
**
** RETURNS
**   Nothing
**
*****************************************************************************/
void
LLLast(
        LIST *list_ptr
      )
{
    void  *dummy_item;

    /*
    ** We need this dummy item because we use LLPopNode to walk the list.
    */
    dummy_item = malloc( (size_t)list_ptr->node_size );

    /*
    ** Start from the beginning of the list
    */
    LLRewind( list_ptr );
    
    /*
    ** Walk the entire list, looking for the one without any data
    */
    while( LLPopNode( list_ptr, dummy_item ) != NULL )
        ;
    
    /*
    ** We should now be pointing to the last, empty element on the list.
    */
    free( dummy_item );
    
}


/*****************************************************************************
**
** NAME
**    LLUpdate - This function will allow the user to update the contents
**               of a particular node on a list.
**
** ARGUMENTS
**               LIST *the_list, void *new_data
**
** DESCRIPTION
**               Given a pointer to a list head structure, this function
**               will free the memory associated with that node's data_ptr,
**               and then copy the data passed in into the space allocated
**               for a new data pointer.
**               This allows the user to change the data that they've pushed
**               onto the linked list, by replacing the old data with the
**               new data.
**
** RETURNS
**   MERGE_SUCCESS or MERGE_FAILURE
**
** LIMITATIONS
**               The user has total control over the data within a particular
**               node.  If the user should push a node that contains space
**               that was malloc'd, then the user has the responsibility to
**               clean up any memory that they allocate.
**               This function will delete memory that the other list
**               functions have created, but not memory that the user may
**               have allocated.
**               Also, it is assumed that the LIST head's node_ptr is
**               pointing to the node which this function is modifying.
**               The user could have accomplished this through LLPopNodeN,
**               or possibly LLPopNode (most likely LLPopNodeN).
*****************************************************************************/
int
LLUpdate(
          LIST *list_ptr,
          void *new_data
        )
{
    void *new_node;

    /*
    ** First, allocate some space for the "new" node.
    */
    new_node = malloc( sizeof( list_ptr->node_size ) ); 
    if ( new_node == NULL )
    {
        return( MERGE_FAILURE );
    }

    /*
    ** Then, copy the new data to the new space.
    */
    memcpy( new_node, new_data, (size_t)list_ptr->node_size );

    /*
    ** Then, free the original node;
    */
    free( list_ptr->node_ptr->data_ptr );

    /*
    ** Finally, make this node pointer's data pointer point to the space
    ** where the new data was copied to.
    */
    list_ptr->node_ptr->data_ptr = new_node;

    return( MERGE_SUCCESS );
}


/*****************************************************************************
**
** NAME
**    LLGetDataPtr - This function will obtain a pointer to a node's data.
**
** ARGUMENTS
**    node_ptr          NODE *    A pointer to a NODE structure
**
** DESCRIPTION
**    This function will take a pointer to a NODE (probably obtained with
**    either LLPopNode or LLPopNodeN) and return a pointer to that
**    particuler node's data.  Thus, with this function, you can get at
**    any node's data so that you can modify the data.
**
** RETURNS
**    A pointer to the node's data, as a type "void".  Upon return, a user can
**    type-cast this pointer to match the data type of the data that was
**    actually pushed onto the linked list.
**
** LIMITATIONS
**    Since this function gives you access to data that is created and
**    manipulated by these linked list functions, you can do many dangerous
**    things.  "With power, comes responsibility."
*****************************************************************************/
void *
LLGetDataPtr(
                NODE *node_ptr
            )
{
    void *data_ptr;

    /*
    ** This is really a very simple function.  We just make data_ptr point
    ** to the internal data_ptr that the node contains.  Then, upon return
    ** the user can type-cast this returned data_ptr to match the data type
    ** of the data that was pushed onto the linked list.
    */
    data_ptr = node_ptr->data_ptr;

    return( data_ptr );
}


/*****************************************************************************
**
** NAME
**    LLDestroy - This function will delete the contents of a list.
**
** ARGUMENTS
**    list_ptr          LIST *    A pointer to a structure of type LIST.
**
** DESCRIPTION
**    This function will take a list that already contains data, and free
**    all memory that has been allocated for the list and its contents.
**
** RETURNS
**   Nothing
**
*****************************************************************************/
void
LLDestroy(
           LIST *list_ptr
         )
{
    NODE *current_node;
    
    /*
    ** Starting from the beginning of the list...
    */
    LLRewind( list_ptr );
    
    /*
    ** Make current node point to the first node.
    */
    current_node = list_ptr->node_ptr;
    
    while( list_ptr->node_ptr != NULL )
    {
        /*
        ** Free the data associated with the current node's data pointer.
        */
        if ( current_node->data_ptr != NULL )
        {
            free( current_node->data_ptr );
        }
    
        /*
        ** Now that we've freed the data, we can free this node.
        ** But first, we make the head node think that it is pointing
        ** to the node that the current is pointing to...
        */
        list_ptr->node_ptr = current_node->next_ptr;

        /*
        ** We then free the current node.
        */
        free( current_node );
  
        /*
        ** Now, we update current node so that it's actually the next node.
        */
        current_node = list_ptr->node_ptr;
        list_ptr->num_elements--;
    }
    
    /*
    ** Now, all of the nodes and their data have been freed, but,
    ** we should make the head node's contents show that fact.
    */
    list_ptr->node_ptr = NULL;
    list_ptr->head_ptr = NULL;
    list_ptr->num_elements = 0;
    list_ptr->node_size = 0;
    
    /*
    ** Remember, don't attempt to free the head node.  We didn't allocate
    ** it.  The create function assumes that the head node was created
    ** outside the scope of these functions!
    */
}


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
char *
Create_Inst_UID()
{
    static short UID_CNTR = 0;
    static char  deviceType[] = "1";
    static char  serial[] = "1";
    static char  Sprnt_uid[68];
    char         creationDate[68];
    char         creationTime[68];
    time_t       timeReturn;
    struct tm*   timePtr;
#ifdef UNIX
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

#ifdef UNIX    
    sprintf(Sprnt_uid, "2.16.840.1.999999.%s.%s.%d.%s.%s.%d", 
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
