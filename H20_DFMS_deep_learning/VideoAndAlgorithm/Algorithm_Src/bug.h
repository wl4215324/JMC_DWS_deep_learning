/*
 * bug.h
 *
 *  Created on: May 9, 2019
 *      Author: tony
 */

#ifndef BUG_H_
#define BUG_H_

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

#ifndef KERNEL_MODE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#define  printk  printf
#define  panic   perror
#endif


#ifndef HAVE_ARCH_BUG
#define BUG() do { \
	printk("kernel BUG at %s:%d!\n", __FILE__, __LINE__); \
	panic("BUG!"); \
} while (0)
#endif

#ifndef HAVE_ARCH_PAGE_BUG
#define PAGE_BUG(page) do { \
	printk("page BUG for page at %p\n", page); \
	BUG(); \
} while (0)
#endif

#ifndef HAVE_ARCH_BUG_ON
#define BUG_ON(condition) do { if (unlikely((condition)!=0)) BUG(); } while(0)
#endif

#ifndef HAVE_ARCH_WARN_ON
#define WARN_ON(condition) do { \
	if (unlikely((condition)!=0)) { \
		printk("Badness in %s at %s:%d\n", __FUNCTION__, __FILE__, __LINE__); \
		dump_stack(); \
	} \
} while (0)
#endif

#endif /* BUG_H_ */
