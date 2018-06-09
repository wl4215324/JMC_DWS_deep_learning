/*
 * timer_delay.h
 *
 *  Created on: Jan 31, 2018
 *      Author: tony
 */

#ifndef TIMER_DELAY_H_
#define TIMER_DELAY_H_

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

static inline void seconds_sleep(unsigned short seconds)
{
    struct timeval tv;
    tv.tv_sec=seconds;
    tv.tv_usec=0;
    int err;

    do
    {
    	err = select(0, NULL, NULL, NULL, &tv);
    }while(err<0 && (errno == EINTR));
}

static inline void milliseconds_sleep(unsigned int mSec)
{
    struct timeval tv;
    tv.tv_sec=mSec/1000;
    tv.tv_usec=(mSec%1000)*1000;
    int err;

    do{
       err=select(0, NULL, NULL, NULL, &tv);
    }while(err<0 && errno==EINTR);
}

static inline void microseconds_sleep(unsigned int uSec)
{
    struct timeval tv;
    tv.tv_sec=uSec/1000000;
    tv.tv_usec=uSec%1000000;
    int err;

    do{
        err=select(0,NULL,NULL,NULL,&tv);
    }while(err<0 && errno==EINTR);
}

#endif /* TIMER_DELAY_H_ */
