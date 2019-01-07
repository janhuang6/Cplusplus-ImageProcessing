/*
 * file:	mppsscu.cc
 * purpose:	MPPS Service Client User to provide the mpps service.
 *
 * inspection history:
 *
 * revision history:
 *   Jiantao Huang		initial version
 */

static const char rcsID[] = "$Id: mppsscu.cc,v 1.6 Exp $";
static const char rcsName[] = "$Name: Atlantis710R01_branch $";
static const char builder[] = _BUILDER_;
const  char  *mppsBuildDate = _BUILDDATE_;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <acquire/acqexception.h>
#include <acquire/mesg.h>
#include <acquire/server.h>
#include <acquire/procexit.h>

#include "mppsimpl.h"

void quitCallback( Mesg *mptr);


int
main( int argc, char *const *argv)
{	
	Server *sp = NULL;
	char *mergeIniFile = NULL;
	char *facilityName = NULL;
	char *mppsName = NULL;

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
	(void)sp->checkOptionValue( argc, argv, 'P', &mppsName);

	/*
	 * advertise our version number
	 */
	sp->setDescription( "MPPS Service Client User");
	sp->setBuildDate( mppsBuildDate);
	if (sp->setReleaseTag( rcsName) < 1)
	{
		sp->setReleaseNumbers( 0, 0, 0, 0 );
		sp->setReleaseType( SW_DEVELOPMENT);
		sp->setBuilder( builder);
	}

	/*
	 * create VersionInfo struct to pass onto the Mppsscu
	 */
	DictionaryPkg::VersionInfo * ver = PortalImpl::buildVersionInfo( componentName, sp );

	/*
	 * set default message parameters
	 */
	sp->getMesg()->setVerbose( toService);
	sp->setCallback( quitCallback, quitCallback);
	sp->Message( MLoverall | toEndUser, "MPPS Service Client User starting.");

	MppsImpl::run( mergeIniFile, mppsName, facilityName, ver );

	delete ver;
	free( mergeIniFile );
	free( facilityName );
	free( mppsName );

	::Message( MSTATUS, MLoverall, "MPPS Service Client User has shutdown cleanly");

	if( sp != NULL)
		sp->exitNow();

	::exit( ProcExitCodeNormal);
}


void
quitCallback( Mesg *mptr)
{
	::Message( MNOTE, MLoverall | toService | toDeveloper, "Attempting to disable further mpps operation.");

	::_exit( ProcExitCodeNormal);

	// FIXME - remove _exit() after fixing segfaults in shutdown code!
	// MppsImpl::shutdown();
}
