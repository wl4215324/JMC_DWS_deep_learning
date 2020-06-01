/*
 * timer.h
 *
 *  Created on: May 8, 2019
 *      Author: tony
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <stddef.h>
#include <sys/time.h>
#include "jiffies.h"
#include "list.h"
#include "bug.h"
//#include "applicfg.h"

#ifndef INT_MAX
#define INT_MAX		((int)(~0U>>1))
#endif

#ifndef INT_MIN
#define INT_MIN		(-INT_MAX - 1)
#endif

#ifndef UINT_MAX
#define UINT_MAX	(~0U)
#endif

#ifndef LONG_MAX
#define LONG_MAX	((long)(~0UL>>1))
#endif

#ifndef LONG_MIN
#define LONG_MIN	(-LONG_MAX - 1)
#endif

#ifndef ULONG_MAX
#define ULONG_MAX	(~0UL)
#endif

#ifndef KERNEL_MODE
#include <pthread.h>
#define  spinlock_t pthread_mutex_t
#define  spin_lock_init(x)  pthread_mutex_init(x, NULL)
#define  SPIN_LOCK_UNLOCKED    PTHREAD_MUTEX_INITIALIZER

#define  spin_lock(x)    pthread_mutex_lock(x)
#define  spin_unlock(x)  pthread_mutex_unlock(x)

#define  spin_lock_irqsave(a, b)     spin_lock(a)
#define  spin_unlock_irqrestore(a,b) spin_unlock(a)

#define  spin_lock_irq(x)   spin_unlock(x)
#define  spin_unlock_irq(x) spin_unlock(x)
#endif

/*
 * per-CPU timer vector definitions:
 */
#define TVN_BITS 6
#define TVR_BITS 8
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)

typedef struct tvec_s {
	struct list_head vec[TVN_SIZE];
} tvec_t;

typedef struct tvec_root_s {
	struct list_head vec[TVR_SIZE];
} tvec_root_t;

struct tvec_t_base_s {
	spinlock_t lock;
	unsigned long long timer_jiffies;
	struct timer_list *running_timer;
	tvec_root_t tv1;
	tvec_t tv2;
	tvec_t tv3;
	tvec_t tv4;
	tvec_t tv5;
} ;

//struct tvec_t_base_s;
typedef struct tvec_t_base_s tvec_base_t;



#define GET_TICKS_TEST ({ \
	struct timeval tv; \
	gettimeofday(&tv, NULL); \
	unsigned long long u_sec = tv.tv_sec; \
	u_sec *= 1000000; \
	u_sec += tv.tv_usec;})

#define per_cpu(var, cpu)  (*((void)cpu, &per_cpu__##var))
#define __get_cpu_var(var) per_cpu__##var


struct timer_list {
	struct list_head entry;
	unsigned long long expires;

	spinlock_t lock;
	unsigned long magic;

	void (*function)(unsigned long);
	unsigned long data;

	struct tvec_t_base_s *base;
};


#define TIMER_MAGIC	0x4b87ad6e

#define TIMER_INITIALIZER(_function, _expires, _data) {		\
		.function = (_function),			\
		.expires = (_expires),				\
		.data = (_data),				\
		.base = NULL,					\
		.magic = TIMER_MAGIC,				\
		.lock = PTHREAD_MUTEX_INITIALIZER, \
	}

#define TVEC_BASE_INITIALIZER(_expires) {		\
		.lock = PTHREAD_MUTEX_INITIALIZER,		\
		.timer_jiffies = _expires,			\
	}

#define DEFINE_PER_CPU(type, name) \
    __typeof__(type) per_cpu__##name

/***
 * init_timer - initialize a timer.
 * @timer: the timer to be initialized
 *
 * init_timer() must be done to a timer prior calling *any* of the
 * other timer functions.
 */
static inline void init_timer(struct timer_list * timer)
{
	timer->base = NULL;
	timer->magic = TIMER_MAGIC;
}

/***
 * timer_pending - is a timer pending?
 * @timer: the timer in question
 *
 * timer_pending will tell whether a given timer is currently pending,
 * or not. Callers must ensure serialization wrt. other operations done
 * to this timer, eg. interrupt contexts, or other CPUs on SMP.
 *
 * return value: 1 if the timer is pending, 0 if not.
 */
static inline int timer_pending(const struct timer_list * timer)
{
	return timer->base != NULL;
}

extern void add_timer_on(struct timer_list *timer, int cpu);
extern int del_timer(struct timer_list * timer);
extern int __mod_timer(struct timer_list *timer, unsigned long expires);
extern int mod_timer(struct timer_list *timer, unsigned long expires);

extern unsigned long next_timer_interrupt(void);

/***
 * add_timer - start a timer
 * @timer: the timer to be added
 *
 * The kernel will do a ->function(->data) callback from the
 * timer interrupt at the ->expired point in the future. The
 * current time is 'jiffies'.
 *
 * The timer's ->expired, ->function (and if the handler uses it, ->data)
 * fields must be set prior calling this function.
 *
 * Timers with an ->expired field in the past will be executed in the next
 * timer tick.
 */
static inline void add_timer(struct timer_list * timer)
{
	__mod_timer(timer, timer->expires);
}

#ifdef CONFIG_SMP
  extern int del_timer_sync(struct timer_list *timer);
  extern int del_singleshot_timer_sync(struct timer_list *timer);
#else
# define del_timer_sync(t) del_timer(t)
# define del_singleshot_timer_sync(t) del_timer(t)
#endif

extern void init_timers(void);
extern void run_local_timers(void);
extern void it_real_fn(unsigned long);

extern DEFINE_PER_CPU(tvec_base_t, tvec_bases);

void init_timers_cpu(int cpu);

void __run_timers(tvec_base_t *base);


#endif /* TIMER_H_ */
