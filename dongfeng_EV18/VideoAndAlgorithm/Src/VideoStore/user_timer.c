/*
 * user_timer.c
 *
 *  Created on: May 14, 2019
 *      Author: tony
 */

#include "user_timer.h"


timer_t timerid;
tvec_base my_tvec_base = TVEC_INITIALIZER(my_tvec_base.next_timer_entry);
//user_timer my_timer = TIMER_INITILIZER(my_timer.entry, 1000, test_fun, 100);


int test_fun(unsigned long data)
{
	DEBUG_INFO(data is: %ld\n, data);
	return 0;
}


static void timer_schedule(tvec_base *base);


void timer_schedule_test(union sigval v)
{
	//DEBUG_INFO(timer_schedule_test \n);
	timer_schedule(&my_tvec_base);
}


int is_timer_detach(user_timer *timer)
{
	return list_empty(&timer->entry);
}


static void init_tvec_base(tvec_base *base)
{
	int i = 0;

	pthread_mutex_init(&base->tvec_base_lock, NULL);

	for(i=0; i<SYS_BITS; i++)
	{
		INIT_LIST_HEAD(base->list_vec+i);
	}

	INIT_LIST_HEAD(&base->next_timer_entry);
	base->next_expires = UL_MAX;
}


void TimerInit(void)
{
	struct sigevent sigev;

	memset (&sigev, 0, sizeof (struct sigevent));
	sigev.sigev_value.sival_int = 0;
	sigev.sigev_notify = SIGEV_THREAD;
	sigev.sigev_notify_attributes = NULL;
	sigev.sigev_notify_function = timer_schedule_test; //timer_notify

	if(timer_create (CLOCK_REALTIME, &sigev, &timerid))
	{
		perror("timer_create()");
	}

	init_tvec_base(&my_tvec_base);
}


void init_user_timer(user_timer *timer, unsigned long long expires, \
		int (*func)(unsigned long), unsigned long data)
{
	INIT_LIST_HEAD(&timer->entry);
	timer->expires = expires;
	DEBUG_INFO(timer->expires: %llu, timer->expires);
	timer->function = func;
	timer->data = data;
	timer->vec_index = 0;
}

// get minimum of all timer
static void get_next_timer_interrupt(tvec_base *base)
{
	int i = 0;
	user_timer *loop, *tmp;
	struct list_head *head;
	unsigned long long expires = UL_MAX; //~0ULL


	pthread_mutex_lock(&base->tvec_base_lock);

	for(i=0; i<SYS_BITS; i++)
	{
		head = base->list_vec + i;

		list_for_each_entry_safe(loop, tmp, head, entry)
		{
			if(expires > loop->expires)
			{
				base->next_expires = loop->expires;
				base->next_timer_entry.next = &loop->entry;
				goto exit_from_next_timer;
			}
		}
	}
    //if no timer found in timer vector
	base->next_expires = expires;
	INIT_LIST_HEAD(&base->next_timer_entry);

exit_from_next_timer:
	pthread_mutex_unlock(&base->tvec_base_lock);
}


static int settime(unsigned long long value)
{
	unsigned long long cur_ticks = 0;
	cur_ticks = GET_TICKS_TEST;
	DEBUG_INFO(value is: %llu cur_ticks: %llu\n, value, cur_ticks);

	if(value <= cur_ticks)
	{
		value = 50;
	}
	else
	{
		value -= cur_ticks;
	}

	DEBUG_INFO(value is: %llu\n, value);
	value = value > UL_MAX ? UL_MAX:value;

	struct itimerspec timerValues;
	time_t tv_nsec = 1000 * (value%1000000);
	time_t tv_sec = value/1000000;

	if(tv_nsec < 0)
	{
		tv_nsec = INT_MAX;
	}

	if(tv_sec < 0)
	{
		tv_sec = INT_MAX;
	}

	timerValues.it_value.tv_sec = tv_sec;
	timerValues.it_value.tv_nsec = tv_nsec;
	timerValues.it_interval.tv_sec = 0;
	timerValues.it_interval.tv_nsec = 0;
	DEBUG_INFO(tv_sec: %d tv_nsec: %d\n, tv_sec, tv_nsec);
 	return timer_settime (timerid, 0, &timerValues, NULL);
}


static int _add_user_timer(tvec_base *base, user_timer *timer)
{
	unsigned int i = 0;
	unsigned long long expires = 0;
	user_timer *loop, *tmp;

	if(!is_timer_detach(timer))
	{
		return -1;
	}

	DEBUG_INFO(timer->expires: %llu\n, timer->expires);
	expires = timer->expires - GET_TICKS_TEST;
	DEBUG_INFO(expires: %llu\n, expires);

	if((signed long long)expires < 0)
	{
		expires = 0;
	}
	else if(expires > UL_MAX)
	{
		expires = UL_MAX;
	}

	DEBUG_INFO(expires: %llu\n, expires);

	while((expires) && (expires >>= 1))
	{
		i++;
	}

	pthread_mutex_lock(&base->tvec_base_lock);
	DEBUG_INFO(i: %d\n, i);
	timer->vec_index = i;

	list_for_each_entry_safe(loop, tmp, base->list_vec+i, entry)
	{
		DEBUG_LINE();

		if(timer->expires < loop->expires)
		{
			DEBUG_LINE();
			list_add_tail(&timer->entry, &loop->entry);
			goto finish_add_timer;
		}
	}

	list_add_tail(&timer->entry, base->list_vec+i);

finish_add_timer:
	pthread_mutex_unlock(&base->tvec_base_lock);
	get_next_timer_interrupt(base);
	DEBUG_INFO(base->next_expires is: %llu\n, base->next_expires);
	settime(base->next_expires);
	return 0;
}


int add_user_timer(user_timer *timer)
{
	return _add_user_timer(&my_tvec_base, timer);
}


static void timer_schedule(tvec_base *base)
{
	user_timer *tmp, *loop;
	unsigned long long jiffies = 0;
	struct list_head *head;
	unsigned long i = 0, time_out_index = 0;

	pthread_mutex_lock(&base->tvec_base_lock);
	jiffies = GET_TICKS_TEST;
//	DEBUG_INFO(jiffies: %llu\n, jiffies);

	if(time_after_eq(jiffies, base->next_expires))
	{
		if(list_empty(&base->next_timer_entry))
		{
			goto wait_next_timeout;
		}

		loop = list_entry(base->next_timer_entry.next, user_timer, entry);
		//DEBUG_INFO();
		loop->function(loop->data);
		time_out_index = loop->vec_index;
		list_del(&loop->entry);
		INIT_LIST_HEAD(&loop->entry);
		INIT_LIST_HEAD(&base->next_timer_entry);

		for(i=0; i <= time_out_index; i++)
		{
			head = base->list_vec + i;

			list_for_each_entry_safe(loop, tmp, head, entry)
			{
				jiffies = GET_TICKS_TEST;

				if(time_after_eq(jiffies, loop->expires))
				{
					loop->function(loop->data);
					list_del(&loop->entry);
					INIT_LIST_HEAD(&loop->entry);
				}
			}
		}
	}

wait_next_timeout:
    pthread_mutex_unlock(&base->tvec_base_lock);
	get_next_timer_interrupt(base);
	DEBUG_INFO(base->next_expires: %llu\n, base->next_expires);
	settime(base->next_expires);
}



static void _detach_user_timer(tvec_base *base, user_timer *timer)
{
	if(is_timer_detach(timer))
	{
		return;
	}

	pthread_mutex_lock(&base->tvec_base_lock);
	list_del(&timer->entry);
	INIT_LIST_HEAD(&timer->entry);
	pthread_mutex_unlock(&base->tvec_base_lock);
	get_next_timer_interrupt(base);
	DEBUG_INFO(base->next_expires: %llu\n, base->next_expires);
	settime(base->next_expires);
}


void detach_user_timer(user_timer *timer)
{
	return _detach_user_timer(&my_tvec_base, timer);
}


static int _modify_user_timer(tvec_base *base, unsigned long new_expires, user_timer *timer)
{
	if(is_timer_detach(timer))
	{
		return -1;
	}

	if(new_expires == timer->expires)
	{
		return 0;
	}

	_detach_user_timer(base, timer);
	timer->expires = new_expires;
	_add_user_timer(base, timer);
	return 0;
}


int modify_user_timer(unsigned long new_expires, user_timer *timer)
{
	return _modify_user_timer(&my_tvec_base, new_expires, timer);
}
