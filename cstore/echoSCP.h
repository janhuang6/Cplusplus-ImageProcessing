#ifndef _ECHOSCP_H_
#define _ECHOSCP_H_ 1

const int THREAD_NORMAL_EXIT=0;
const int THREAD_EXCEPTION=1;

#ifdef linux

/*
 * Boolean enumerator
 */
typedef enum
{
    TRUE = 1,
    FALSE = 0
} BOOLEAN;

struct ECHO_ARGS
{
    int     EchoIncomingPort;
	int     ApplicationID;
    bool    Verbose;
};

/*
 * Note, having a maximum of 10 child servers is an arbitrary selection.
 * A higher number can be placed here if need be.
 */
#define MAX_CHILD_SERVERS 10

static int shandler (   int                 Asigno,
                        long                Acode,
                        struct sigcontext*  Ascp );
static void sigint_routine (
                        int                 Asigno,
                        long                Acode,
                        struct sigcontext*  Ascp );
static void reaper(     int                 Asigno,
                        long                Acode,
                        struct sigcontext*  Ascp );

/*
 *  Module function prototypes
 */
void* echoSCP( void* echo_args );

static void Handle_EchoAssociation(MC_SOCKET mcSocket, 
								   int applicationID, 
								   BOOLEAN childReject);
#endif

const char* GetSyntaxDescription(TRANSFER_SYNTAX A_syntax);

void PrintError(const char* A_string, MC_STATUS A_status);

#endif
