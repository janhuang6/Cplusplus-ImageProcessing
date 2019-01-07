/*
 * file:	worklistserver.cc
 * purpose:	WorkList server to provide worklist information.
 *
 * inspection history:
 *
 * revision history:
 *   Jiantao Huang		initial version
 */

static const char rcsID[] = "$Id: worklistserver.cc,v 1.12 Exp $";
static const char rcsName[] = "$Name: Atlantis710R01_branch $";

static const char builder[] = _BUILDER_;
const  char  *worklistBuildDate = _BUILDDATE_;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <acquire/mesg.h>
#include <acquire/server.h>
#include <acquire/procexit.h>

#include "worklistimpl.h"

void quitCallback( Mesg *mptr);

int
main( int argc, char *const *argv)
{	
	Server *sp = NULL;
	char *mergeIniFile = NULL;
	char *facilityName = NULL;
	char *worklistName = NULL;

	/*
	 * become a server
	 */
	if(( sp = Server::instance( argc, argv)) == NULL)
	{
		Message( MALARM, toEndUser | toService | MLoverall, "Acquisition Server cannot start.");
		::exit( ProcExitCodeError);
	}
	(void)sp->checkOptionValue( argc, argv, 'M', &mergeIniFile);
	(void)sp->checkOptionValue( argc, argv, 'F', &facilityName);
	(void)sp->checkOptionValue( argc, argv, 'W', &worklistName);

	/*
	 * advertise our version number
	 */
	sp->setDescription( "WorkList Server");
	sp->setBuildDate( worklistBuildDate);
	if (sp->setReleaseTag( rcsName) < 1)
	{
		sp->setReleaseNumbers( 0, 0, 0, 0 );
		sp->setReleaseType( SW_DEVELOPMENT);
		sp->setBuilder( builder);
	}

	/*
	 * create VersionInfo struct to pass onto the Worklist
	 */
	DictionaryPkg::VersionInfo * ver = PortalImpl::buildVersionInfo( componentName, sp );

	/*
	 * set default message parameters
	 */
	sp->getMesg()->setVerbose( toService);
	sp->setCallback( quitCallback, quitCallback);
	sp->Message( MLoverall | toEndUser, "WorkList Server starting.");

	WorklistImpl::run( mergeIniFile, worklistName, facilityName, ver );

	delete ver;
	free( mergeIniFile );
	free( facilityName );
	free( worklistName );

	::Message( MSTATUS, MLoverall, "Worklist Server has shutdown cleanly");

	if( sp != NULL)
		sp->exitNow();

	::exit( ProcExitCodeNormal);
}


void
quitCallback( Mesg *mptr)
{
	::Message( MNOTE, MLoverall | toService | toDeveloper, "Attempting to disable further worklist operation.");
	
	::_exit( ProcExitCodeNormal);

	// FIXME - remove _exit() after fixing segfaults in shutdown code!
	// WorklistImpl::shutdown();
}
