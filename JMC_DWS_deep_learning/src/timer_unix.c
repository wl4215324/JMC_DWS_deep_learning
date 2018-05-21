/*
 * timer_unix.c
 *
 *  Created on: Nov 10, 2017
 *      Author: tony
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#include "applicfg.h"
#include "user_timer.h"

static pthread_mutex_t CanFestival_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct timeval last_sig;
static timer_t timer;

void TimerCleanup(void)
{
	/* only used in realtime apps */
}

void EnterMutex(void)
{
	if(pthread_mutex_lock(&CanFestival_mutex))
	{
		fprintf(stderr, "pthread_mutex_lock() failed\n");
	}
}

void LeaveMutex(void)
{
	if(pthread_mutex_unlock(&CanFestival_mutex))
	{
		fprintf(stderr, "pthread_mutex_unlock() failed\n");
	}
}

void timer_notify(sigval_t val)
{
	if(gettimeofday(&last_sig,NULL))
	{
		perror("gettimeofday()");
	}

	EnterMutex();
	TimeDispatch();
	LeaveMutex();
}


void TimerInit(void)
{
	struct sigevent sigev;

	// Take first absolute time ref.
	if(gettimeofday(&last_sig, NULL))
	{
		perror("gettimeofday()");
	}

#if defined(__UCLIBC__)
	int ret;
	ret = timer_create(CLOCK_PROCESS_CPUTIME_ID, NULL, &timer);
	signal(SIGALRM, timer_notify);
#else
	memset (&sigev, 0, sizeof (struct sigevent));
	sigev.sigev_value.sival_int = 0;
	sigev.sigev_notify = SIGEV_THREAD;
	sigev.sigev_notify_attributes = NULL;
	sigev.sigev_notify_function = timer_notify;

	if(timer_create (CLOCK_REALTIME, &sigev, &timer))
	{
		perror("timer_create()");
	}
#endif
}


void StopTimerLoop(TimerCallback_t exitfunction)
{
	EnterMutex();

	if(timer_delete (timer))
	{
		perror("timer_delete()");
	}

	exitfunction(NULL,0);
	LeaveMutex();
}

void StartTimerLoop(TimerCallback_t init_callback)
{
	EnterMutex();
	// At first, TimeDispatch will call init_callback.
	SetAlarm(NULL, 0, init_callback, 0, 0);
	LeaveMutex();
}


#define maxval(a,b) ((a>b)?a:b)

void setTimer(TIMEVAL value)
{
	long tv_nsec = 1000 * (maxval(value,1)%1000000);
	time_t tv_sec = value/1000000;
	struct itimerspec timerValues;
	timerValues.it_value.tv_sec = tv_sec;
	timerValues.it_value.tv_nsec = tv_nsec;
	timerValues.it_interval.tv_sec = 0;
	timerValues.it_interval.tv_nsec = 0;

 	timer_settime (timer, 0, &timerValues, NULL);
}

TIMEVAL getElapsedTime(void)
{
	struct timeval p;

	if(gettimeofday(&p,NULL))
	{
		perror("gettimeofday()");
	}

	return (p.tv_sec - last_sig.tv_sec)* 1000000 + p.tv_usec - last_sig.tv_usec;
}

