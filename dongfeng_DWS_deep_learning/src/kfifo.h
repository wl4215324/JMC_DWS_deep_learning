#ifndef _Linux_KFIFO_H
#define _Linux_KFIFO_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)


#define __u32 unsigned long
#define __u64 unsigned long long

#define DWS_WARNING_FIFO_SIZE  256

#define min(x,y) ({\
        typeof(x) _x = (x);\
        typeof(y) _y = (y);\
        (void) (&_x == &_y);\
        _x < _y ? _x : _y; })

#define max(x,y) ({\
        typeof(x) _x = (x);\
        typeof(y) _y = (y);\
        (void) (&_x == &_y);\
        _x > _y ? _x : _y; })

#if 0
#define BUG() do { \
    printf("kernel BUG at %s:%d!\n", __FILE__, __LINE__); \
    panic("BUG!"); \
} while (0)
#endif

#define assert(p) do {  \
	if (!(p)) { \
		printf("BUG at %s:%d assert(%s)\n",   \
			   __FILE__, __LINE__, #p);         \
	}       \
} while (0)


#ifdef BUG_ON
#define BUG_ON(cond) assert(!cond)
#else
#define BUG_ON(condition)  do {\
	if (unlikely((condition)!=0)) {\
		printf("Badness in %s at %s:%d\n",__FUNCTION__, __FILE__, __LINE__);\
	}\
} while(0)

#endif


static inline bool is_power_of_2(unsigned long n)
{
	return (n != 0 && ((n & (n -1 )) == 0));
}

static inline unsigned long roundup_pow_of_two(unsigned long x)
{
	unsigned int position = 0;
	unsigned int i;

	if(x == 0)
		return 0;

	for(i=x; i != 0; i >>=1 )
		position++;

	return (unsigned int)(1 << position);
}

struct kfifo {
    unsigned char *buffer;    /* the buffer holding the data */
    unsigned int size;        /* the size of the allocated buffer */
    unsigned int in;          /* data is added at offset (in % size) */
    unsigned int out;         /* data is extracted from off. (out % size) */
};

struct kfifo *kfifo_init(unsigned char *buffer, unsigned int size);
struct kfifo *kfifo_alloc(unsigned int size);
void kfifo_free(struct kfifo *fifo);
unsigned int __kfifo_put(struct kfifo *fifo, unsigned char *buffer, unsigned int len);
unsigned int __kfifo_get(struct kfifo *fifo, unsigned char *buffer, unsigned int len);

static inline void __kfifo_reset(struct kfifo *fifo)
{
    fifo->in = fifo->out = 0;
}

static inline void kfifo_reset(struct kfifo *fifo)
{

    __kfifo_reset(fifo);

}

static inline unsigned int kfifo_put(struct kfifo *fifo,
                     unsigned char *buffer, unsigned int len)
{
    unsigned int ret;

    ret = __kfifo_put(fifo, buffer, len);

    return ret;
}

static inline unsigned int kfifo_get(struct kfifo *fifo,
                     unsigned char *buffer, unsigned int len)
{
    unsigned int ret;

    ret = __kfifo_get(fifo, buffer, len);

    if (fifo->in == fifo->out)
        fifo->in = fifo->out = 0;


    return ret;
}

static inline unsigned int __kfifo_len(struct kfifo *fifo)
{
    return fifo->in - fifo->out;
}

static inline unsigned int kfifo_len(struct kfifo *fifo)
{
    unsigned int ret;

    ret = __kfifo_len(fifo);

    return ret;
}

extern struct kfifo *dws_warn_fifo;

#endif
