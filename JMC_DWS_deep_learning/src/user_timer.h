/*
 * timer.h
 *
 *  Created on: Nov 10, 2017
 *      Author: tony
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "timerscfg.h"
#include "applicfg.h"
#include "driving_behav_analys.h"


#define TIMER_HANDLE INTEGER16


/* --------- types and constants definitions --------- */
#define TIMER_FREE 0
#define TIMER_ARMED 1
#define TIMER_TRIG 2
#define TIMER_TRIG_PERIOD 3

#define TIMER_NONE -1
#define MAX_NB_TIMER  32

typedef TimerFlag CO_Data;

typedef void (*TimerCallback_t)(CO_Data* d, TimerEventType id);

struct struct_s_timer_entry
{
	UNS8 state;
	CO_Data* d;
	TimerCallback_t callback; /* The callback func. */
	TimerEventType id; /* The callback func. */
	TIMEVAL val;
	TIMEVAL interval; /* Periodicity */
};

typedef struct struct_s_timer_entry s_timer_entry;

/* ---------  prototypes --------- */
/*#define SetAlarm(d, id, callback, value, period) printf("%s, %d, SetAlarm(%s, %s, %s, %s, %s)\n",__FILE__, __LINE__, #d, #id, #callback, #value, #period); _SetAlarm(d, id, callback, value, period)*/
/**
 * @ingroup timer
 * @brief Set an alarm to execute a callback function when expired.
 * @param *d Pointer to a CAN object data structure
 * @param id The alarm Id
 * @param callback A callback function
 * @param value Call the callback function at current time + value
 * @param period Call periodically the callback function
 * @return handle The timer handle
 */
TIMER_HANDLE SetAlarm(CO_Data* d, TimerEventType id, TimerCallback_t callback, TIMEVAL value, TIMEVAL period);

/**
 * @ingroup timer
 * @brief Delete an alarm before expiring.
 * @param handle A timer handle
 * @return The timer handle
 */
TIMER_HANDLE DelAlarm(TIMER_HANDLE handle);

void TimeDispatch(void);

/**
 * @ingroup timer
 * @brief Set a timerfor a given time.
 * @param value The time value.
 */
void setTimer(TIMEVAL value);

/**
 * @ingroup timer
 * @brief Get the time elapsed since latest timer occurence.
 * @return time elapsed since latest timer occurence
 */
TIMEVAL getElapsedTime(void);

void free_all_alarm();

void free_spec_type_alarm(TimerEventType id);

#endif /* TIMER_H_ */
