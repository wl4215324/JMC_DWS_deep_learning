/*
 * timer.c
 *
 *  Created on: May 8, 2019
 *      Author: tony
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "timer.h"



unsigned long long jiffies;


static inline void set_running_timer(tvec_base_t *base,
					struct timer_list *timer)
{
#ifdef CONFIG_SMP
	base->running_timer = timer;
#endif
}



///* Fake initialization */
DEFINE_PER_CPU(tvec_base_t, tvec_bases) = {.lock = PTHREAD_MUTEX_INITIALIZER,};

static void check_timer_failed(struct timer_list *timer)
{
	static int whine_count;

	if (whine_count < 16)
	{
		whine_count++;
		printk("Uninitialised timer!\n");
		printk("This is just a warning.  Your computer is OK\n");
		printk("function=0x%p, data=0x%lx\n",
			timer->function, timer->data);
		//dump_stack();
	}

	/*
	 * Now fix it up
	 */
	spin_lock_init(&timer->lock);
	timer->magic = TIMER_MAGIC;
}


static inline void check_timer(struct timer_list *timer)
{
	if (timer->magic != TIMER_MAGIC)
		check_timer_failed(timer);
}


static void internal_add_timer(tvec_base_t *base, struct timer_list *timer)
{
	unsigned long expires = timer->expires;
	unsigned long idx = expires - base->timer_jiffies;
	struct list_head *vec;

//	DEBUG_INFO(expires: %ld idx: %ld\n, expires, idx);

	if (idx < TVR_SIZE)
	{
		int i = expires & TVR_MASK;
		vec = base->tv1.vec + i;
		DEBUG_LINE();
	}
	else if (idx < 1 << (TVR_BITS + TVN_BITS))
	{
		int i = (expires >> TVR_BITS) & TVN_MASK;
		vec = base->tv2.vec + i;
//		DEBUG_INFO(i: %d\n, i);
	}
	else if (idx < 1 << (TVR_BITS + 2 * TVN_BITS))
	{
		int i = (expires >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
		vec = base->tv3.vec + i;
		DEBUG_LINE();
	}
	else if (idx < 1 << (TVR_BITS + 3 * TVN_BITS))
	{
		int i = (expires >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK;
		vec = base->tv4.vec + i;
//		DEBUG_INFO(i: %d vec:%8x\n, i, vec);
		DEBUG_LINE();
	}
	else if((signed long) idx < 0)
	{
		/*
		 * Can happen if you add a timer with expires == jiffies,
		 * or you set a timer to go off in the past
		 */
		vec = base->tv1.vec + (base->timer_jiffies & TVR_MASK);
		DEBUG_LINE();
	}
	else
	{
		int i;
		/* If the timeout is larger than 0xffffffff on 64-bit
		 * architectures then we use the maximum timeout:
		 */
		if (idx > 0xffffffffUL)
		{
			idx = 0xffffffffUL;
			expires = idx + base->timer_jiffies;
		}

		i = (expires >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
		vec = base->tv5.vec + i;
		DEBUG_LINE();
	}

	/*
	 * Timers are FIFO:
	 */
	list_add_tail(&timer->entry, vec);
}

int __mod_timer(struct timer_list *timer, unsigned long expires)
{
	tvec_base_t *old_base, *new_base;
	int ret = 0;

	check_timer(timer);

	spin_lock_irqsave(&timer->lock, flags);
	new_base = &__get_cpu_var(tvec_bases);

	printf("%s, %d line\n", __FILE__, __LINE__);

repeat:
	old_base = timer->base;
//	printf("%s, %d line timer->base: %8x, old_base: %8x \n", __FILE__, __LINE__, new_base, old_base);

	/*
	 * Prevent deadlocks via ordering by old_base < new_base.
	 */
	if (old_base && (new_base != old_base))
	{
		if (old_base < new_base)
		{
			spin_lock(&new_base->lock);
			spin_lock(&old_base->lock);
		}
		else
		{
			spin_lock(&old_base->lock);
			spin_lock(&new_base->lock);
		}
		/*
		 * The timer base might have been cancelled while we were
		 * trying to take the lock(s):
		 */
		if (timer->base != old_base)
		{
			spin_unlock(&new_base->lock);
			spin_unlock(&old_base->lock);
			goto repeat;
		}
	}
	else
	{
//		printf("%s, %d line timer->base: %8x, old_base: %8x \n", __FILE__, __LINE__, timer->base, old_base);
		spin_lock(&new_base->lock);

		if (timer->base != old_base)
		{
			spin_unlock(&new_base->lock);
			goto repeat;
		}
	}

	/*
	 * Delete the previous timeout (if there was any), and install
	 * the new one:
	 */
	if (old_base)
	{
		list_del(&timer->entry);
		ret = 1;
	}

	timer->expires = expires;
//	DEBUG_INFO(new_base is: %8x timer: %8x\n, new_base, timer);
	internal_add_timer(new_base, timer);
	timer->base = new_base;


	if (old_base && (new_base != old_base))
		spin_unlock(&old_base->lock);
	spin_unlock(&new_base->lock);
	spin_unlock_irqrestore(&timer->lock, flags);

	return ret;
}

/***
 * add_timer_on - start a timer on a particular CPU
 * @timer: the timer to be added
 * @cpu: the CPU to start it on
 *
 * This is not very scalable on SMP. Double adds are not possible.
 */
void add_timer_on(struct timer_list *timer, int cpu)
{
	tvec_base_t *base = &per_cpu(tvec_bases, cpu);
  	//unsigned long flags;

  	BUG_ON(timer_pending(timer) || !timer->function);

	check_timer(timer);

	spin_lock_irqsave(&base->lock, flags);
	internal_add_timer(base, timer);
	timer->base = base;
	spin_unlock_irqrestore(&base->lock, flags);
}


/***
 * mod_timer - modify a timer's timeout
 * @timer: the timer to be modified
 *
 * mod_timer is a more efficient way to update the expire field of an
 * active timer (if the timer is inactive it will be activated)
 *
 * mod_timer(timer, expires) is equivalent to:
 *
 *     del_timer(timer); timer->expires = expires; add_timer(timer);
 *
 * Note that if there are multiple unserialized concurrent users of the
 * same timer, then mod_timer() is the only safe way to modify the timeout,
 * since add_timer() cannot modify an already running timer.
 *
 * The function returns whether it has modified a pending timer or not.
 * (ie. mod_timer() of an inactive timer returns 0, mod_timer() of an
 * active timer returns 1.)
 */
int mod_timer(struct timer_list *timer, unsigned long expires)
{
	check_timer(timer);

	/*
	 * This is a common optimization triggered by the
	 * networking code - if the timer is re-modified
	 * to be the same thing then just return:
	 */
	if (timer->expires == expires && timer_pending(timer))
		return 1;

	return __mod_timer(timer, expires);
}

/***
 * del_timer - deactive a timer.
 * @timer: the timer to be deactivated
 *
 * del_timer() deactivates a timer - this works on both active and inactive
 * timers.
 *
 * The function returns whether it has deactivated a pending timer or not.
 * (ie. del_timer() of an inactive timer returns 0, del_timer() of an
 * active timer returns 1.)
 */
int del_timer(struct timer_list *timer)
{
//	unsigned long flags;
	tvec_base_t *base;

	check_timer(timer);

repeat:
 	base = timer->base;
	if (!base)
		return 0;

	spin_lock_irqsave(&base->lock, flags);

	if (base != timer->base)
	{
		spin_unlock_irqrestore(&base->lock, flags);
		goto repeat;
	}

	list_del(&timer->entry);
	/* Need to make sure that anybody who sees a NULL base also sees the list ops */
	//smp_wmb();
	timer->base = NULL;
	spin_unlock_irqrestore(&base->lock, flags);

	return 1;
}

#ifdef CONFIG_SMP
/***
 * del_timer_sync - deactivate a timer and wait for the handler to finish.
 * @timer: the timer to be deactivated
 *
 * This function only differs from del_timer() on SMP: besides deactivating
 * the timer it also makes sure the handler has finished executing on other
 * CPUs.
 *
 * Synchronization rules: callers must prevent restarting of the timer,
 * otherwise this function is meaningless. It must not be called from
 * interrupt contexts. The caller must not hold locks which would prevent
 * completion of the timer's handler.  Upon exit the timer is not queued and
 * the handler is not running on any CPU.
 *
 * The function returns whether it has deactivated a pending timer or not.
 *
 * del_timer_sync() is slow and complicated because it copes with timer
 * handlers which re-arm the timer (periodic timers).  If the timer handler
 * is known to not do this (a single shot timer) then use
 * del_singleshot_timer_sync() instead.
 */
int del_timer_sync(struct timer_list *timer)
{
	tvec_base_t *base;
	int i, ret = 0;

	check_timer(timer);

del_again:
	ret += del_timer(timer);

	for_each_online_cpu(i) {
		base = &per_cpu(tvec_bases, i);
		if (base->running_timer == timer) {
			while (base->running_timer == timer) {
				cpu_relax();
				preempt_check_resched();
			}
			break;
		}
	}
	smp_rmb();
	if (timer_pending(timer))
		goto del_again;

	return ret;
}
EXPORT_SYMBOL(del_timer_sync);

/***
 * del_singleshot_timer_sync - deactivate a non-recursive timer
 * @timer: the timer to be deactivated
 *
 * This function is an optimization of del_timer_sync for the case where the
 * caller can guarantee the timer does not reschedule itself in its timer
 * function.
 *
 * Synchronization rules: callers must prevent restarting of the timer,
 * otherwise this function is meaningless. It must not be called from
 * interrupt contexts. The caller must not hold locks which wold prevent
 * completion of the timer's handler.  Upon exit the timer is not queued and
 * the handler is not running on any CPU.
 *
 * The function returns whether it has deactivated a pending timer or not.
 */
int del_singleshot_timer_sync(struct timer_list *timer)
{
	int ret = del_timer(timer);

	if (!ret) {
		ret = del_timer_sync(timer);
		BUG_ON(ret);
	}

	return ret;
}
EXPORT_SYMBOL(del_singleshot_timer_sync);
#endif

static int cascade(tvec_base_t *base, tvec_t *tv, int index)
{
	/* cascade all the timers from tv up one level */
	struct list_head *head, *curr;

//	DEBUG_INFO(tv addr: %8x index: %d\n, tv->vec, index);
	//DEBUG_INFO(tv addr: %8x index: %d\n, tv->vec+index, index);

	//list_replace_init(tv->vec + index, head);
	head = tv->vec + index;
	curr = head->next;

//	DEBUG_INFO(tv addr: %8x index: %d\n, tv->vec+index, index);

	/*
	 * We are removing _all_ timers from the list, so we don't  have to
	 * detach them individually, just clear the list afterwards.
	 */
	while (curr != head)
	{
		struct timer_list *tmp;

		tmp = list_entry(curr, struct timer_list, entry);
		BUG_ON(tmp->base != base);
		curr = curr->next;
		internal_add_timer(base, tmp);
	}

	return index;
}

/***
 * __run_timers - run all expired timers (if any) on this CPU.
 * @base: the timer vector to be processed.
 *
 * This function cascades all vectors and executes all expired timer
 * vectors.
 */
#define INDEX(N) (base->timer_jiffies >> (TVR_BITS + N * TVN_BITS)) & TVN_MASK

 void __run_timers(tvec_base_t *base)
{
	struct timer_list *timer;
	unsigned long long jiffies = GET_TICKS_TEST;

	spin_lock_irq(&base->lock);

//	DEBUG_INFO(jiffies: %ld base->timer_jiffies: %ld \n, jiffies, base->timer_jiffies);

	while (time_after_eq(jiffies, base->timer_jiffies))
	{
		struct list_head work_list = LIST_HEAD_INIT(work_list);
		struct list_head *head = &work_list;
 		int index = base->timer_jiffies & TVR_MASK;

// 		DEBUG_INFO(i: %d index: %ld\n, i++, index);

		/*
		 * Cascade timers:
		 */
		if ( !index &&
			(!cascade(base, &base->tv2, INDEX(0))) && \
		    (!cascade(base, &base->tv3, INDEX(1))) && !cascade(base, &base->tv4, INDEX(2)))
			cascade(base, &base->tv5, INDEX(3));

		++base->timer_jiffies;
		list_splice_init(base->tv1.vec + index, &work_list);

repeat:
		if (!list_empty(head))
		{
			void (*fn)(unsigned long);
			unsigned long data;

			timer = list_entry(head->next,struct timer_list,entry);
 			fn = timer->function;
 			data = timer->data;

// 			DEBUG_INFO(data: %d\n, data);

			list_del(&timer->entry);
			set_running_timer(base, timer);
			//smp_wmb();
			timer->base = NULL;

			spin_unlock_irq(&base->lock);
			{
//			    u32 preempt_count = preempt_count();
				fn(data);
//				if (preempt_count != preempt_count()) {
//					printk("huh, entered %p with %08x, exited with %08x?\n", fn, preempt_count, preempt_count());
//					BUG();
//				}
			}

			spin_lock_irq(&base->lock);
			goto repeat;
		}
	}

	set_running_timer(base, NULL);
	spin_unlock_irq(&base->lock);
}


/*
 * Find out when the next timer event is due to happen. This
 * is used on S/390 to stop all activity when a cpus is idle.
 * This functions needs to be called disabled.
 */
unsigned long next_timer_interrupt(void)  //find minimum expires in all of timers
{
	tvec_base_t *base;
	struct list_head *list;
	struct timer_list *nte;
	unsigned long expires;
	tvec_t *varray[4];
	int i, j;

	base = &__get_cpu_var(tvec_bases);
	spin_lock(&base->lock);
	expires = base->timer_jiffies + (LONG_MAX >> 1);
	list = 0;

	/* Look for timer events in tv1. */
	j = base->timer_jiffies & TVR_MASK;

	/* find minimum of expires in beginning 256 timers */
	do
	{
		list_for_each_entry(nte, base->tv1.vec + j, entry)
		{
			expires = nte->expires;

			if (j < (base->timer_jiffies & TVR_MASK))
				list = base->tv2.vec + (INDEX(0));
			goto found;
		}

		j = (j + 1) & TVR_MASK;
	} while (j != (base->timer_jiffies & TVR_MASK));

	/* Check tv2-tv5. */
	varray[0] = &base->tv2;
	varray[1] = &base->tv3;
	varray[2] = &base->tv4;
	varray[3] = &base->tv5;

	for (i = 0; i < 4; i++)
	{
		j = INDEX(i);

		do
		{
			if (list_empty(varray[i]->vec + j))
			{
				j = (j + 1) & TVN_MASK;
				continue;
			}

			list_for_each_entry(nte, varray[i]->vec + j, entry)
				if (time_before(nte->expires, expires))
					expires = nte->expires;

			if (j < (INDEX(i)) && i < 3)
				list = varray[i + 1]->vec + (INDEX(i + 1));
			goto found;
		} while (j != (INDEX(i)));
	}

found:
	if (list)
	{
		/*
		 * The search wrapped. We need to look at the next list
		 * from next tv element that would cascade into tv element
		 * where we found the timer element.
		 */
		list_for_each_entry(nte, list, entry)
		{
			if (time_before(nte->expires, expires))
				expires = nte->expires;
		}
	}

	spin_unlock(&base->lock);
	return expires;
}


void init_timers_cpu(int cpu)
{
	int j;
	tvec_base_t *base;

	base = &per_cpu(tvec_bases, cpu);
	spin_lock_init(&base->lock);

	for (j = 0; j < TVN_SIZE; j++)
	{
		INIT_LIST_HEAD(base->tv5.vec + j);
		INIT_LIST_HEAD(base->tv4.vec + j);
		INIT_LIST_HEAD(base->tv3.vec + j);
		INIT_LIST_HEAD(base->tv2.vec + j);
	}

	//DEBUG_INFO(base->tv2.vec addr: %8x\n, base->tv2.vec);

	for (j = 0; j < TVR_SIZE; j++)
		INIT_LIST_HEAD(base->tv1.vec + j);

	//base->timer_jiffies = jiffies;
	base->timer_jiffies = GET_TICKS_TEST;
}


