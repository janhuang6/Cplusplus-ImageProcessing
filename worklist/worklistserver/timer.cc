/*
 * file:	timer.cc
 * purpose:	Implementation of the timer class
 *              Modified from book "Unix System Programming Using C++"
 * created:	30-Nov-03
 * property of:	Philips Nuclear Medicine, ADAC Laboratories
 *
 * revision history:
 *   30-Nov-03	Jiantao Huang		    initial version
 */

#include "timer.h"
#include "acquire/mesg.h"

/* constructor: setup a timer */
Timer::Timer(int signo, SIGFUNC action, int timer_tag, clock_t sys_clock)
{
	_status = 0;

	struct sigaction sigv;
	sigemptyset(&sigv.sa_mask);
	sigv.sa_flags = SA_SIGINFO;
	sigv.sa_sigaction = action;
	if (sigaction( signo, &sigv, 0) == -1)
	{	::Message( MALARM, toEndUser | toService | MLoverall, "sigaction failed");
		_status = errno;
	}
	else
	{
		struct sigevent sigx;
		memset(&sigx, 0, sizeof(struct sigevent));
		sigx.sigev_notify = SIGEV_SIGNAL;
		sigx.sigev_signo = signo;
		sigx.sigev_value.sival_int = timer_tag;

		if (timer_create( sys_clock, &sigx, &_timerID)==-1)
		{	::Message( MALARM, toEndUser | toService | MLoverall, "timer_create failed");
			_status = errno;
		}
	}
}
		
/* destructor: discard a timer */
Timer::~Timer() throw()
{
	if(_status == 0)
	{
		stop();
		if(timer_delete( _timerID) == -1)
			::Message( MALARM, toEndUser | toService | MLoverall, "timer_delete failed");
	}
}

/* Check timer _status */
int Timer::operator!()
{
	return _status? 1:0;
}

/* setup a relative time timer */
int Timer::run(long start_sec, long start_nsec, long reload_sec, long reload_nsec)
{
	if(_status)
		return -1;
	_val.it_value.tv_sec		= start_sec;
	_val.it_value.tv_nsec		= start_nsec;
	_val.it_interval.tv_sec		= reload_sec;
	_val.it_interval.tv_nsec	= reload_nsec;
	if (timer_settime( _timerID, 0, &_val, 0) == -1)
	{
		::Message( MALARM, toEndUser | toService | MLoverall, "timer_settime failed");
		_status = errno;
		return -1;
	}
	return 0;
}

/* setup an absolute time timer */
int Timer::run(time_t start_time, long reload_sec, long reload_nsec)
{
	if(_status)
		return -1;
	_val.it_value.tv_sec		= start_time;
	_val.it_value.tv_nsec		= 0;
	_val.it_interval.tv_sec		= reload_sec;
	_val.it_interval.tv_nsec	= reload_nsec;
	if (timer_settime( _timerID, TIMER_ABSTIME, &_val, 0) == -1)
	{
		::Message( MALARM, toEndUser | toService | MLoverall, "timer_settime failed");
		_status = errno;
		return -1;
	}
	return 0;
}

/* Stop a timer from running */
int Timer::stop()
{
	if(_status)
		return -1;
	_val.it_value.tv_sec		= 0;
	_val.it_value.tv_nsec		= 0;
	_val.it_interval.tv_sec		= 0;
	_val.it_interval.tv_nsec	= 0;
	if (timer_settime( _timerID, 0, &_val, 0) == -1)
	{
		::Message( MALARM, toEndUser | toService | MLoverall, "timer_settime failed");
		_status = errno;
		return -1;
	}
	return 0;
}

/* Get timer overrun statistic */
int Timer::overrun()
{
	if(_status)
		return -1;
	return timer_getoverrun( _timerID);
}

/* Get timer remaining time to expiration */
int Timer::values( long& sec, long& nsec)
{
	if(_status)
		return -1;
	if(timer_gettime( _timerID, &_val ) == -1)
	{
		::Message( MALARM, toEndUser | toService | MLoverall, "timer_gettime failed");
		_status = errno;
		return -1;
	}
	sec = _val.it_value.tv_sec;
	nsec = _val.it_value.tv_nsec;
	return 0;
}

/* Overload << operator for timer objects */
ostream& operator<<( ostream& os, Timer& obj)
{
        long sec, nsec;
        obj.values( sec, nsec );
        double tval = sec+((double)nsec/1000000000.0);
        os << "time left: " << tval << std::endl;
        ::Message( MALARM, toEndUser | toService | MLoverall, "time left: %f\n", tval);
        return os;
}
