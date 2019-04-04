/*
 * producer_consumer_queue.h
 *
 *  Created on: Feb 20, 2019
 *      Author: tony
 */

#ifndef PRODUCER_CONSUMER_QUEUE_H_
#define PRODUCER_CONSUMER_QUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>

#define MAX_BUFFER_SIZE 10
#define SHM_MODE 0600
#define SHM_MAX_SIZE  4096
#define SEM_MODE 0600

#define SEM_NUM 3
#define SEM_FULL_INDEX 0
#define SEM_EMPTY_INDEX 1
#define MUTEX_INDEX 2

#ifndef BYTE
#define BYTE unsigned char
#endif


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


typedef struct
{
	unsigned int in;
	unsigned int out;
	unsigned int size;
} ShmSegment;


typedef struct
{
	int shm_id;
	int sem_id;
	ShmSegment *shm_segment;
	void *shm_data;
} shmfifo;

union semun {
	int val ; /* for SETVAL */
	struct semid_ds * buf ; /* for IPC_STAT and IPC_SET */
	ushort * array ; /* for GETALL and SETALL */
} ;

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


extern shmfifo *shmfifo_init(key_t key, unsigned int shm_size);
extern unsigned int shmfifo_put(shmfifo *p_shmfifo, const BYTE *wr_buffer, unsigned int wr_len);
extern unsigned int shmfifo_get(shmfifo *p_shmfifo, BYTE *rd_buffer, unsigned int rd_len);
extern void shmfifo_destory(shmfifo *p_shmfifo);
extern void shmfifo_reset(shmfifo *p_shmfifo);
extern unsigned int shmfifo_len(shmfifo *p_shmfifo);

#endif /* PRODUCER_CONSUMER_QUEUE_H_ */
