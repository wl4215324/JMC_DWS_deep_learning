/*
 * applicfg.h
 *
 *  Created on: Nov 10, 2017
 *      Author: tony
 */

#ifndef APPLICFG_H_
#define APPLICFG_H_

#ifndef __KERNEL__
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#else
#include <linux/types.h>
#endif

/*  Define the architecture : little_endian or big_endian
 -----------------------------------------------------
 Test :
 UNS32 v = 0x1234ABCD;
 char *data = &v;

 Result for a little_endian architecture :
 data[0] = 0xCD;
 data[1] = 0xAB;
 data[2] = 0x34;
 data[3] = 0x12;

 Result for a big_endian architecture :
 data[0] = 0x12;
 data[1] = 0x34;
 data[2] = 0xAB;
 data[3] = 0xCD;
 */

/* Integers */
#define INTEGER8 int8_t
#define INTEGER16 int16_t
#define INTEGER24 int32_t
#define INTEGER32 int32_t
#define INTEGER40 int64_t
#define INTEGER48 int64_t
#define INTEGER56 int64_t
#define INTEGER64 int64_t

/* Unsigned integers */
#define UNS8   u_int8_t
#define UNS16  u_int16_t
#define UNS32  u_int32_t
#define UNS24  u_int32_t
#define UNS40  u_int64_t
#define UNS48  u_int64_t
#define UNS56  u_int64_t
#define UNS64  u_int64_t

/* Reals */
#define REAL32	float
#define REAL64 double

/* Definition of error and warning macros */
/* -------------------------------------- */
#ifdef __KERNEL__
#	define MSG(...) printk (__VA_ARGS__)
//#elif defined USE_RTAI
//#	define MSG(...) rt_printk (__VA_ARGS__)
#elif defined USE_XENO
#	define MSG(...)
#else
#	include <stdio.h>
#	define MSG(...) printf (__VA_ARGS__)
#endif

/* Definition of MSG_ERR */
/* --------------------- */
#ifdef DEBUG_ERR_CONSOLE_ON
#    define MSG_ERR(num, str, val)            \
          MSG("%s,%d : 0X%x %s 0X%x \n",__FILE__, __LINE__,num, str, val);
#else
#    define MSG_ERR(num, str, val)
#endif

/* Definition of MSG_WAR */
/* --------------------- */
#ifdef DEBUG_WAR_CONSOLE_ON
#    define MSG_WAR(num, str, val)          \
          MSG("%s,%d : 0X%x %s 0X%x \n",__FILE__, __LINE__,num, str, val);
#else
#    define MSG_WAR(num, str, val)
#endif

#endif /* APPLICFG_H_ */
