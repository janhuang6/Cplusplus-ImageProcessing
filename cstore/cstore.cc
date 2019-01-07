/*
 * file:	cstore.cc
 * purpose:	cstore program to store the DICOM images into PACs.
 *          It supports storage only, storage with synchronous
 *          commitment, and storage with asynchronous commitment.
 *
 * inspection history:
 *
 * revision history:
 *   Jiantao Huang		initial version
 */

static const char rcsName[] = "$Name: Atlantis710R01_branch $";
static const char builder[] = _BUILDER_;
const  char  *cstoreBuildDate = _BUILDDATE_;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <acquire/mesg.h>
#include <acquire/server.h>
#include <acquire/procexit.h>

#include "DICOMstorageimpl.h"

void quitCallback( Mesg *mptr);

int main( int argc, char *const *argv)
{	
	Server *sp = NULL;
	char *mergeIniFile = NULL;
	char *cstoreName = NULL;

	/*
	 * become a server
	 */
	if(( sp = Server::instance( argc, argv)) == NULL)
	{
		Message( MALARM, toEndUser | toService | MLoverall, "Cstore acquisition Server cannot start.");
		::exit( ProcExitCodeError);
	}
	(void)sp->checkOptionValue( argc, argv, 'M', &mergeIniFile);
	(void)sp->checkOptionValue( argc, argv, 'S', &cstoreName);

	/*
	 * advertise our version number
	 */
	sp->setDescription( "cstore");
	sp->setBuildDate( cstoreBuildDate);
	if (sp->setReleaseTag( rcsName) < 1)
	{
		sp->setReleaseNumbers( 0, 0, 0, 0 );
		sp->setReleaseType( SW_DEVELOPMENT);
		sp->setBuilder( builder);
	}

	/*
	 * create VersionInfo struct to pass onto the Cstore
	 */
	DictionaryPkg::VersionInfo * ver = PortalImpl::buildVersionInfo( componentName, sp );

	/*
	 * set default message parameters
	 */
	sp->getMesg()->setVerbose( toService);
	sp->setCallback( quitCallback, quitCallback);
	sp->Message( MLoverall | toEndUser, "Cstore Server starting.");

	DICOMStorageImpl::run( mergeIniFile, cstoreName, ver );

	delete ver;
	free( mergeIniFile );
	free( cstoreName );

	::Message( MSTATUS, MLoverall, "Cstore Server has shutdown cleanly");

	if( sp != NULL)
		sp->exitNow();

	::exit( ProcExitCodeNormal);
}

void quitCallback( Mesg *mptr)
{
	::Message( MNOTE, MLoverall | toService | toDeveloper, "Attempting to disable further cstore operation.");
// FIXME - remove _exit() after fixing segfaults in shutdown code!
//	DICOMStorageImpl::shutdown();
	::_exit( ProcExitCodeNormal);
}
