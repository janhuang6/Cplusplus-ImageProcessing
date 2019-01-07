/*
 * file:	manualworklist.cc
 * purpose:	Implementation of the manualworklist class
 * created:	06-Nov-03
 * property of:	Philips Nuclear Medicine, ADAC Laboratories
 *
 * revision history:
 *   26-Nov-03	Jiantao Huang		    initial version
 *   10-Feb-04	Jiantao Huang		    Add mergeIniFile
 */

#include <unistd.h>
#include <thread.h>

#include "control/lookupmatchutils.h"
#include "manualworklist.h"

ManualWorklist::ManualWorklist( const char *mergeIniFile )
                :WorklistManager( mergeIniFile )
{
}

ManualWorklist::~ManualWorklist() throw()
{
}

int ManualWorklist::sendWorklistQuery(
            const DictionaryPkg::LookupMatchList& toMatch
            )
{	MutexGuard guard (_lockManualQuery);

	pthread_t tid;
	pthread_attr_t attr;

	_myLookupMatchList = toMatch;

	pthread_attr_init(&attr); // Initialize with the default value
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if( pthread_create(&tid, &attr, _sendWorklistQuery, (void *)this) != 0) {
		Message( MWARNING, "#Software error: cannot create thread to query manualworklist info. Skip and wait for the next query.");
		guard.release();
		return 0;
	}

	guard.release();
	return 1;
}

QueryType ManualWorklist::getQueryType()
{
	return DictionaryPkg::QUERYTYPE_MANUAL;
}
