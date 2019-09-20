/*
 * user_timer.h
 *
 *  Created on: May 14, 2019
 *      Author: tony
 */

#ifndef USER_TIMER_H_
#define USER_TIMER_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include "list.h"
#include "timer.h"
#include "applicfg.h"

#define  SYS_BITS  32
//#define  UL_MAX    (unsigned long long)((1ULL << SYS_BITS) -1)
#define  UL_MAX    (unsigned long long)(~0ULL -1)


typedef struct {
	pthread_mutex_t tvec_base_lock;
	struct list_head list_vec[SYS_BITS];
	struct list_head next_timer_entry;
	unsigned long long next_expires;
} tvec_base;



typedef struct {
	struct list_head entry;
	unsigned long long expires;
	int (*function)(unsigned long);
	unsigned int data;
	unsigned int vec_index;
} user_timer;


#define  TVEC_INITIALIZER(_entry) {\
	.tvec_base_lock = PTHREAD_MUTEX_INITIALIZER, \
	.next_timer_entry = {&_entry, &_entry}, \
	.next_expires = UL_MAX, \
}

#define TIMER_INITILIZER(_entry, _expires, _func, _data) { \
	.entry = {&_entry, &_entry}, \
	.expires = _expires, \
	.function = _func, \
	.data = _data, \
}

extern tvec_base my_tvec_base;
extern user_timer my_timer;

int test_fun(unsigned long data);

void TimerInit(void);

void init_user_timer(user_timer *timer, unsigned long long expires, int (*func)(unsigned long),\
		unsigned long data);

int add_user_timer(user_timer *timer);

//void get_next_timer_interrupt(tvec_base *base);

//void timer_schedule(tvec_base *base);

int modify_user_timer(unsigned long new_expires, user_timer *timer);

void detach_user_timer(user_timer *timer);

#endif /* USER_TIMER_H_ */
