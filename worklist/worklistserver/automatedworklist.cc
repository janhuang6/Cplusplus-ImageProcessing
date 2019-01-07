/*
 * file:	automatedworklist.cc
 * purpose:	Implementation of the automatedworklist class
 * created:	06-Nov-03
 * property of:	Philips Nuclear Medicine, ADAC Laboratories
 *
 * revision history:
 *   26-Nov-03	Jiantao Huang		    initial version
 *   04-Feb-04	Jiantao Huang		    add method processTimerParameters
 *   10-Feb-04	Jiantao Huang		    Add mergeIniFile
 *   24-Nov-04  Jingwen "Gene" Wang     Visibroker 6.0 and Forte 6.2 Compiler upgrades
 */

#include <unistd.h>
#include <thread.h>
#include <math.h>

#include <acquire/acqexception.h>
#include "control/lookupmatchutils.h"
#include "automatedworklist.h"

#define TIMER_TAG 9

AutomatedWorklist* g_ptrToAutomatedWorklist;

AutomatedWorklist::AutomatedWorklist( const char *mergeIniFile )
                   :WorklistManager( mergeIniFile )
{  _ptrTimer = NULL;

   MutexGuard guard (_lockLookupMatchList);

// These lines of code are modified from testConnectFacility.cc
   try {
        Message( MNOTE, MLoverall | toService | toDeveloper, "Attempting to get the Automated Worklist Query Defaults");
	DictionaryPkg::LookupMatchList * autoList = _connection->getWorklistAutoQueryDefaults();
	if ( !autoList )
		throw AcqException( "will be caught immediately and converted to NUCMED exception" );
	_myLookupMatchList = *autoList;
   } catch (... ) {
	Message( MWARNING, MLoverall | toService | toDeveloper, "Failed to getAutomatedWorklistQueryDefaults.");
        throw ( DictionaryPkg::NucMedException( DictionaryPkg::NUCMED_SOFTWARE ) );
   }

   processTimerParameters();

   guard.release();

   _ptrTimer = new Timer(SIGUSR1, _timerFunc, TIMER_TAG);
   if (!_ptrTimer || !(*_ptrTimer))
      {
        Message( MWARNING, MLoverall | toService | toDeveloper, "Failed to create the timer.");
        exit(1);
      }
}

AutomatedWorklist::~AutomatedWorklist() throw()
{
   // Stop the timer
   if (_ptrTimer && !(*_ptrTimer)==0)
   {   delete _ptrTimer;
       _ptrTimer = NULL;
   }
}

void AutomatedWorklist::setQueryDefaults(const DictionaryPkg::LookupMatchList& queryParameter)
{  MutexGuard guard (_lockLookupMatchList);
   _myLookupMatchList = queryParameter;
   
   /* Previous runTimer parameters */
//   char startTime_pre[9];
//   long intervalSec_pre;
//   boolean autoQueryOn_pre;

//   strcpy(startTime_pre, _startTime);
//   intervalSec_pre = _intervalSec;
//   autoQueryOn_pre = _autoQueryOn;

   processTimerParameters();
   guard.release();

//   if( (_autoQueryOn != autoQueryOn_pre) ||
//       (_intervalSec != intervalSec_pre) ||
//       strcmp(_startTime, startTime_pre) )
   runTimer();
}

void AutomatedWorklist::_timerFunc(int signo, siginfo_t* evp, void* ucontext)
{   pthread_t tid;
    pthread_attr_t attr;

    if(evp->si_value.sival_int != TIMER_TAG) {
        ::Message( MALARM, toEndUser | toService | MLoverall, "Get timer expiration signal but with unexpected TIMER_TAG. Looking for %d but getting %d. Ignored.", TIMER_TAG, evp->si_value.sival_int);
        return;
    }

    if( !_checkDayofTheWeek() )
        return;
    if( !_checkTimeofTheDay() )
        return;

    pthread_attr_init(&attr); // Initialize with the default value
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if( pthread_create(&tid, &attr, _sendWorklistQuery,
                       (void *)g_ptrToAutomatedWorklist) != 0) {
        ::Message( MALARM, toEndUser | toService | MLoverall, "#Software error: cannot create thread to query automatedworklist info. Skip and wait for the next query.");
        return;
    }

    return;
}

char AutomatedWorklist::_checkDayofTheWeek()
{
   time_t tim;
   struct tm* do_time;
   char str[26];

   tim = time(0); // current time
   do_time = localtime(&tim);

/* tm_wday: days since Sunday - [0, 6] */
   if ((g_ptrToAutomatedWorklist->_daysOfWeek>>do_time->tm_wday) & 0x01)
      return 1;

   strcpy(str, ctime(&tim));
   str[3]='\0';
   ::Message( MALARM, toEndUser | toService | MLoverall, "Skip automated query on %s as requested. Current time is %s", str, str+4);

   return 0;
}

char AutomatedWorklist::_checkTimeofTheDay()
{
   time_t tim, tim2, tim_2;
   char abs_tm[26], abs_tm2[26], abs_tm_2[26], rel_tm[9], rel_tm2[9], rel_tm_2[9];

   tim = time(0); // current time
   tim2 = tim+2; // allow 2 seconds difference in case of 5:59:59
   tim_2 = tim-2; // allow 2 seconds difference in case of 18:00:01
   strcpy(abs_tm, ctime(&tim));
   strcpy(abs_tm2, ctime(&tim2));
   strcpy(abs_tm_2, ctime(&tim_2));

   strncpy(rel_tm, abs_tm+11, 8);
   rel_tm[8]='\0';
   strncpy(rel_tm2, abs_tm2+11, 8);
   rel_tm2[8]='\0';
   strncpy(rel_tm_2, abs_tm_2+11, 8);
   rel_tm_2[8]='\0';

   ::Message( MNOTE, toEndUser | toService | MLoverall, "Current time of the day is: %s\n", rel_tm);

   if(strcmp(rel_tm_2, g_ptrToAutomatedWorklist->_stopTime)>0) {
      ::Message( MALARM, toEndUser | toService | MLoverall, "Time(%s) is after the _stopTime(%s). Skip automated query.", rel_tm, g_ptrToAutomatedWorklist->_stopTime);
     return 0;
   }

   if(strcmp(rel_tm2, g_ptrToAutomatedWorklist->_startTime)<0) {
      ::Message( MALARM, toEndUser | toService | MLoverall, "Time(%s) is before the _startTime(%s). Skip automated query.", rel_tm, g_ptrToAutomatedWorklist->_startTime);
      return 0;
   }

   return 1;
}

QueryType AutomatedWorklist::getQueryType()
{
	return DictionaryPkg::QUERYTYPE_AUTOMATED;
}

void AutomatedWorklist::runTimer()
{
   // Stop the current timer
   if (_ptrTimer && !(*_ptrTimer)==0)
      _ptrTimer->stop();

   if(_autoQueryOn == false) {
	Message( MNOTE, toEndUser | toService | MLoverall, "Automated query is set off. Timer is stopped.");
	return;
   }

   time_t tim;
   struct tm* do_time;

   tim = time(0); // current time
   Message( MNOTE, toEndUser | toService | MLoverall, "Current time is: %s", ctime(&tim));
   do_time=localtime(&tim);
   do_time->tm_hour = atoi(_startTime);
   do_time->tm_min = atoi(&(_startTime[3]));
   do_time->tm_sec = atoi(&(_startTime[6]));
   tim = mktime(do_time);
   Message( MNOTE, toEndUser | toService | MLoverall, "Start the Timer using start time: %s, ", _startTime);
   Message( MNOTE, toEndUser | toService | MLoverall, "repeat interval: %ld seconds\n", _intervalSec);

   if (_ptrTimer && !(*_ptrTimer)==0)
//_ptrTimer->run(tim, 1, 0);
      _ptrTimer->run(tim, _intervalSec, 0);
}

void AutomatedWorklist::processTimerParameters()
{	const char *pc;
	char traitName[128];
	double loEnd, hiEnd;

        // Set the default values
        strcpy(_startTime, "06:00:00");
        strcpy(_stopTime, "18:00:00");
        _intervalSec = 3 * 60 * 60; // default 3 hours
        _numberOfDaysToQuery = 1; // default value
        _autoQueryOn = false;
        _daysOfWeek = 62; // default value Mon-Fri 0011,1110 = 0x3e = 62

	for(int i=0; i<(int)_myLookupMatchList.length(); i++)
	{	memset(traitName, 0, sizeof(traitName));

		try {
                        pc=_ptrToTraits->getName(_myLookupMatchList[i].field);
                }
		catch (...) {
			Message( MALARM, toEndUser | toService | MLoverall, "TraitID %ld not found in TraitAttrList.", (long)_myLookupMatchList[i].field);
			continue;
		}
                if(pc==NULL) {
			Message( MALARM, toEndUser | toService | MLoverall, "TraitUtils::getName(%ld) returns NULL pointer.", (long)_myLookupMatchList[i].field);
			continue;
		}
                else
			strcpy(traitName, pc);

		if(!strcmp(traitName, "TRAIT_MWKL_TIMER_START_TIME"))
		{	// This trait is for the Daily Timer Start Time
			pc = _myMatchUtils->getString(_myLookupMatchList[i]);
			if( pc==NULL )
				Message( MALARM, toEndUser | toService | MLoverall, 
					 "Unable to extract the value from TraitMatch _myLookupMatchList[%d].",
					 i);
			else
				strcpy(_startTime, pc);
		}
		else if(!strcmp(traitName, "TRAIT_MWKL_TIMER_STOP_TIME"))
		{	// This trait is for the Daily Timer Stop Time
			pc = _myMatchUtils->getString(_myLookupMatchList[i]);
			if( pc==NULL )
				Message( MALARM, toEndUser | toService | MLoverall, 
					 "Unable to extract the value from TraitMatch _myLookupMatchList[%d].",
					 i);
			else
				strcpy(_stopTime, pc);
		}
		else if(!strcmp(traitName, "TRAIT_MWKL_TIMER_REPEAT_INTERVAL"))
		{	// This trait is for the Timer Repeat Interval
			if( 0==_myMatchUtils->getRange( _myLookupMatchList[i], &loEnd, &hiEnd) ||
			    fabs(loEnd - hiEnd) > 0.0001 )
				Message( MALARM, toEndUser | toService | MLoverall, 
					 "Unable to extract a unique numeric value from TraitMatch _myLookupMatchList[%d].",
					 i);
			else
				_intervalSec = (long) (loEnd * 60 + 0.5); // convert into seconds
		}
		else if(!strcmp(traitName, "TRAIT_MWKL_QUERY_RELATIVE_DAYS"))
		{	// This trait means to query for this number of days starting today
			if( 0==_myMatchUtils->getRange( _myLookupMatchList[i], &loEnd, &hiEnd) ||
			    fabs(loEnd - hiEnd) > 0.0001 )
				Message( MALARM, toEndUser | toService | MLoverall, 
					 "Unable to extract a unique numeric value from TraitMatch _myLookupMatchList[%d].",
					 i);
			else
				_numberOfDaysToQuery = (int) (loEnd + 0.5);
		}
		else if(!strcmp(traitName, "TRAIT_MWKL_AUTOMATED_QUERY"))
		{	// This trait says automated query on/off
			if(0!=_myMatchUtils->getExpected(_myLookupMatchList[i]))
				_autoQueryOn = true;
		}
		else if(!strcmp(traitName, "TRAIT_MWKL_DAYS_OF_WEEK"))
		{	// This trait says what days of a week to start the automated query
			if( 0==_myMatchUtils->getRange( _myLookupMatchList[i], &loEnd, &hiEnd) ||
			    fabs(loEnd - hiEnd) > 0.0001  )
				Message( MALARM, toEndUser | toService | MLoverall, 
					 "Unable to extract a unique numeric value from TraitMatch _myLookupMatchList[%d].",
					 i);
			else
				_daysOfWeek = (unsigned char) (loEnd + 0.5);
		}
        }
}
