#ifndef _MANUALWORKLIST_H_
#define _MANUALWORKLIST_H_

/*
 * file:	manualworklist.h
 * purpose:	ManualWorklist class
 * created:	26-Nov-03
 * property of:	Philips Nuclear Medicine, ADAC Laboratories
 *
 * revision history:
 *   26-Nov-03	Jiantao Huang		    initial version
 *   10-Feb-04	Jiantao Huang		    Add mergeIniFile
 */

#include "worklistmanager.h"

class ManualWorklist : public WorklistManager {
	ThreadMutex _lockManualQuery; /* Serialize access to the critical section.*/

	QueryType getQueryType();

	// Disallow copying or assignment.
	ManualWorklist(const ManualWorklist&);
	ManualWorklist& operator=(const ManualWorklist&);

public:
	ManualWorklist( const char *mergeIniFile);
	~ManualWorklist() throw();

        int sendWorklistQuery(
            const DictionaryPkg::LookupMatchList& toMatch
            );
};

#endif
