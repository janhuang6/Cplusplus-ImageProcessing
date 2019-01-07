#ifndef _AUTOMATEDWORKLIST_H_
#define _AUTOMATEDWORKLIST_H_

/*
 * file:	automatedworklist.h
 * purpose:	AutomatedWorklist class
 * created:	26-Nov-03
 * property of:	Philips Nuclear Medicine, ADAC Laboratories
 *
 * revision history:
 *   26-Nov-03	Jiantao Huang		    initial version
 *   04-Feb-04	Jiantao Huang		    add method processAutomatedQueryParameter
 *   10-Feb-04	Jiantao Huang		    Add mergeIniFile
 *   24-Nov-04  Jingwen "Gene" Wang     Visibroker 6.0 and Forte 6.2 Compiler upgrades
 */

#include "worklistmanager.h"
#include "timer.h"

class AutomatedWorklist : public WorklistManager {
	Timer *_ptrTimer;
	char _startTime[9];
	char _stopTime[9];
	long _intervalSec;        /* Query repeat time interval */
	bool _autoQueryOn;	  /* a boolean to turn on/off the auto query */
	unsigned char _daysOfWeek;/* bit0-bit6 for Sunday-Saturday */

	static char _checkDayofTheWeek();
	static char _checkTimeofTheDay();
	static void _timerFunc(int signo, siginfo_t* evp, void* ucontext);
	QueryType getQueryType();
	void processTimerParameters();

	// Disallow copying or assignment.
	AutomatedWorklist(const AutomatedWorklist&);
	AutomatedWorklist& operator=(const AutomatedWorklist&);

public:
	AutomatedWorklist( const char *mergeIniFile);
	~AutomatedWorklist() throw();

        void setQueryDefaults(const DictionaryPkg::LookupMatchList& queryParameter);

	void runTimer();
};

extern AutomatedWorklist* g_ptrToAutomatedWorklist;

#endif
