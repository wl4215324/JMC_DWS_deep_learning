/*
 * timerscfg.h
 *
 *  Created on: Nov 10, 2017
 *      Author: tony
 */

#ifndef TIMERSCFG_H_
#define TIMERSCFG_H_

#include <pthread.h>

/* Time unit : us */
/* Time resolution : 64bit (~584942 years) */
#define TIMEVAL unsigned long long
#define TIMEVAL_MAX ~(TIMEVAL)0
#define MS_TO_TIMEVAL(ms) ms*1000L
#define US_TO_TIMEVAL(us) us
#define S_TO_TIMEVAL(secd) (TIMEVAL)(secd*1000000L)
#define MIN_TO_TIMEVAL(min) (min*60*1000000L)

#define TASK_HANDLE pthread_t

#endif /* TIMERSCFG_H_ */
