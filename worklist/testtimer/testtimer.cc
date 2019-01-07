/*
 * file:	timertest.cc
 * purpose:	To test the Timer class
 *
 * revision history:
 *   Jiantao Huang		    initial version
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "timer.h"

#define TIMER_TAG 8

void timerfunc(int signo, siginfo_t* evp, void* ucontext)
{   
    if(evp->si_value.sival_int != TIMER_TAG) {
        printf("Get timer expiration signal but with unexpected TIMER_TAG. Looking for %d but getting %d. Ignored.\n", TIMER_TAG, evp->si_value.sival_int);
        return;
    }

    time_t tim = time(0); // current time
    printf("Timerfunc called. Current time is: %s", ctime(&tim));
}

int
main( int argc, char *const *argv)
{  long interval_sec;
   time_t tim;
   struct tm do_time;

   if( argc<4 ) {
       printf("Usage: testtimer MM/DD/YYYY HH:MM:SS interval_sec\n");
       return 0;
   }

   tim = time(0); // current time
   printf("Program starts at time: %s", ctime(&tim));

   do_time.tm_mon = atoi(argv[1])-1;
   do_time.tm_mday = atoi(strchr(argv[1], '/')+1);
   do_time.tm_year = atoi(strrchr(argv[1], '/')+1)-1900;
   do_time.tm_hour = atoi(argv[2]);
   do_time.tm_min = atoi(strchr(argv[2], ':')+1);
   do_time.tm_sec = atoi(strrchr(argv[2], ':')+1);

   tim = mktime(&do_time);
   printf("Use Timer Start Time: %s", ctime(&tim));

   interval_sec = atoi(argv[3]);
   printf("Use Timer Repeat Interval: %ld seconds\n", interval_sec);

   Timer myTimer(SIGUSR1, timerfunc, TIMER_TAG);
   myTimer.run(tim, interval_sec, 0);

   while(1);

   return 1;
}
