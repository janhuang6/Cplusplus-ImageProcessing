/*
 * file:	timer.h
 * purpose:	The Timer class
 *              Modified from book "Unix System Programming Using C++"
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */

#ifndef TIMER_H
#define TIMER_H

#include <signal.h>
#include <time.h>
#include <errno.h>
#include <iostream>
using std::ostream;

typedef void (*SIGFUNC)(int, siginfo_t*, void*);

class Timer {

	timer_t _timerID;
	int	_status;
	struct itimerspec _val;

	// Disallow copying or assignment.
	Timer(const Timer&);
	Timer& operator=(const Timer&);

public:

	/* constructor: setup a timer */
	Timer(int signo, SIGFUNC action, int timer_tag, clock_t sys_clock = CLOCK_REALTIME);

	/* destructor: discard a timer */
	~Timer() throw();

	/* Check timer status */
	int operator!();

	/* setup a relative time timer */
	int run(long start_sec, long start_nsec, long reload_sec, long reload_nsec);
	/* setup an absolute time timer */
	int run(time_t start_time, long reload_sec, long reload_nsec);

	/* Stop a timer from running */
	int stop();

	/* Get timer overrun statistic */
	int overrun();
	/* Get timer remaining time to expiration */
	int values( long& sec, long& nsec);
	/* Overload << operator for Timer objects */
	friend ostream& operator<<( ostream& os, Timer& obj);
};

ostream& operator<<( ostream& os, Timer& obj);

#endif
