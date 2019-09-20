/*
 * producer_consumer_for_progress.c
 *
 *  Created on: Feb 20, 2019
 *      Author: tony
 */

#include "producer_consumer_shmfifo.h"

//sem_id 表示信号量集合的 id
//sem_num 表示要处理的信号量在信号量集合中的索引
//P操作
void waitSem(int sem_id, int sem_num)
{
	struct sembuf sb;
	sb.sem_num = sem_num;
	sb.sem_op = -1; //表示要把信号量减一
	//sb.sem_flg = SEM_UNDO;//
	sb.sem_flg = 0;
	//第二个参数是 sembuf [] 类型的，表示数组
	//第三个参数表示 第二个参数代表的数组的大小
	if(semop(sem_id, &sb, 1) < 0)
	{
		perror("waitSem failed");
	}
}


//V操作
void sigSem(int sem_id, int sem_num)
{
	struct sembuf sb;
	sb.sem_num = sem_num;
	sb.sem_op = 1; //表示要把信号量 plus one
	//sb.sem_flg = SEM_UNDO;
	sb.sem_flg = 0;
	//第二个参数是 sembuf [] 类型的，表示数组
	//第三个参数表示 第二个参数代表的数组的大小
	if(semop(sem_id, &sb, 1) < 0)
	{
		perror("sigSem failed");
	}
}


shmfifo *shmfifo_init(key_t key, unsigned int shm_size)
{
	char *shmptr = NULL;
	int shmid = 0;
	shmfifo *p_shmfifo = (shmfifo*)malloc(sizeof(shmfifo));
	int app_shm_size = 0;
	union semun sem_union;

	if(!p_shmfifo)
	{
		//return (void*)(-1);
		goto error_exit_init;
	}

	if(-1 == (shmid = shmget(key, 0, 0))) //shared memory is not existent before, need to be created here
	{
		if(!is_power_of_2(shm_size))
		{
			app_shm_size = roundup_pow_of_two(shm_size);
		}

		app_shm_size += sizeof(ShmSegment);
		shmid = shmget(key, app_shm_size, IPC_CREAT|0644);

		if(-1 == shmid)
		{
			perror("shmget error:");
			free(p_shmfifo);
			goto error_exit_init;
		}

		p_shmfifo->shm_id = shmid;
		shmptr = (void*)shmat(p_shmfifo->shm_id, NULL, 0);

		if(*shmptr == -1)
		{
			perror("shmat error: ");
			free(p_shmfifo);
			goto error_exit_init;
		}

		p_shmfifo->shm_segment = (ShmSegment*)shmptr;
		p_shmfifo->shm_segment->in = 0;
		p_shmfifo->shm_segment->out = 0;
		p_shmfifo->shm_segment->size = app_shm_size - sizeof(ShmSegment);
		p_shmfifo->shm_data = (void*)p_shmfifo->shm_segment + sizeof(ShmSegment);

		p_shmfifo->sem_id = semget(key, SEM_NUM, IPC_CREAT|0644); //创建三个信号量，SEM_FULL, SEM_EMPTY和MUTEX

		if(p_shmfifo->sem_id == -1)
		{
			perror("semget error:");
			shmdt((void*)p_shmfifo->shm_segment);
			free(p_shmfifo);
			goto error_exit_init;
		}

		sem_union.val = 0;
		if(semctl(p_shmfifo->sem_id ,SEM_FULL_INDEX, SETVAL, sem_union) == -1)
		{	//将索引为0的信号量设置为0-->SEM_FULL
			perror("semctl set value error ");
			//goto error_exit_init;
		}

		sem_union.val = 1;
		if(semctl(p_shmfifo->sem_id, SEM_EMPTY_INDEX, SETVAL, sem_union) == -1)
		{	//将索引为1的信号量设置为1-->SEM_EMPTY
		 	perror("semctl set value error ");
		 	//goto error_exit_init;
		}

		sem_union.val = 1;
		if(semctl(p_shmfifo->sem_id, MUTEX_INDEX, SETVAL, sem_union) == -1)
		{	//将索引为3的信号量设置为1-->MUTEX
		 	perror("semctl set value error ");
		 	//goto error_exit_init;
		}
	}
	else  //already exist before
	{
		p_shmfifo->shm_id = shmid;
		p_shmfifo->sem_id = semget(key, SEM_NUM, IPC_CREAT|0644);
		p_shmfifo->shm_segment = (ShmSegment*)shmat(p_shmfifo->shm_id, NULL, 0);
		p_shmfifo->shm_data = (char*)p_shmfifo->shm_segment + sizeof(ShmSegment);
	}

	return p_shmfifo;

error_exit_init:
    return (void*)(-1);
}


unsigned int shmfifo_put(shmfifo *p_shmfifo, const BYTE *wr_buffer, unsigned int len)
{
	unsigned int l;
	unsigned int i = 0;

	if((!p_shmfifo) || (!wr_buffer))
	{
		return -1;
	}

	waitSem(p_shmfifo->sem_id, MUTEX_INDEX);

    len = min(len, (p_shmfifo->shm_segment->size - p_shmfifo->shm_segment->in + p_shmfifo->shm_segment->out));
    l = min(len, p_shmfifo->shm_segment->size - (p_shmfifo->shm_segment->in & (p_shmfifo->shm_segment->size - 1)));

    //printf("\n len: %ld, l: %ld \n", len, l);

    memcpy(p_shmfifo->shm_data + (p_shmfifo->shm_segment->in & (p_shmfifo->shm_segment->size - 1)), wr_buffer, l);

//    printf("\nfirst put: ");
//    for(i=0; i<l; i++)
//    {
//    	printf("  %d", *(BYTE*)(p_shmfifo->shm_data + (p_shmfifo->shm_segment->in & (p_shmfifo->shm_segment->size - 1))+i));
//    }

    memcpy(p_shmfifo->shm_data, wr_buffer + l, len - l);

//    printf("\nsecond put: ");
//    for(i=0; i<len - l; i++)
//    {
//    	printf("  %d", *(BYTE*)(p_shmfifo->shm_data+i));
//    }

    p_shmfifo->shm_segment->in += len;
    //printf("\n p_shmfifo->shm_segment->in: %ld\n", p_shmfifo->shm_segment->in);

	sigSem(p_shmfifo->sem_id, MUTEX_INDEX);
	return len;
}


unsigned int shmfifo_get(shmfifo *p_shmfifo, BYTE *rd_buffer, unsigned int len)
{
	unsigned int l;
	unsigned int i = 0;

	if((!p_shmfifo) || (!rd_buffer))
	{
		return -1;
	}

	waitSem(p_shmfifo->sem_id, MUTEX_INDEX);

    len = min(len, (p_shmfifo->shm_segment->in - p_shmfifo->shm_segment->out));
    printf("%ld, len: %d\n", p_shmfifo->shm_segment->in - p_shmfifo->shm_segment->out, len);
    l = min(len, p_shmfifo->shm_segment->size - (p_shmfifo->shm_segment->out & (p_shmfifo->shm_segment->size-1)));

    memcpy(rd_buffer, p_shmfifo->shm_data + (p_shmfifo->shm_segment->out&(p_shmfifo->shm_segment->size - 1)), l);
//    printf("\n frist get(%ld): ", l);
//    for(i=0; i<l; i++)
//    {
//    	printf("  %d", *(BYTE*)(p_shmfifo->shm_data + (p_shmfifo->shm_segment->out & (p_shmfifo->shm_segment->size - 1))+i));
//    }

    memcpy(rd_buffer+l, p_shmfifo->shm_data, len-l);
//    printf("\n second get(%ld): ", (len-l));
//    for(i=0; i<len-l; i++)
//    {
//    	printf("  %d", *(BYTE*)(p_shmfifo->shm_data+i));
//    }

    p_shmfifo->shm_segment->out += len;
    //printf("\n get out: %ld", p_shmfifo->shm_segment->out);

    if(p_shmfifo->shm_segment->in == p_shmfifo->shm_segment->out)
    {
    	p_shmfifo->shm_segment->in = p_shmfifo->shm_segment->out = 0;
    }

	sigSem(p_shmfifo->sem_id, MUTEX_INDEX);
	return len;
}


void shmfifo_destory(shmfifo *p_shmfifo)
{
	if(!p_shmfifo)
	{
		return;
	}

	shmdt(p_shmfifo->shm_segment);
	shmctl(p_shmfifo->shm_id, IPC_RMID, 0);
	semctl(p_shmfifo->sem_id, IPC_RMID, 0);
	free(p_shmfifo);
}


void shmfifo_reset(shmfifo *p_shmfifo)
{
	if(!p_shmfifo)
	{
		return;
	}

	waitSem(p_shmfifo->sem_id, MUTEX_INDEX);

	memset(p_shmfifo->shm_data, 0, p_shmfifo->shm_segment->size);
	p_shmfifo->shm_segment->in = 0;
	p_shmfifo->shm_segment->out = 0;
	p_shmfifo->shm_segment->size = 0;

	sigSem(p_shmfifo->sem_id, MUTEX_INDEX);
}

unsigned int shmfifo_len(shmfifo *p_shmfifo)
{
	return (p_shmfifo->shm_segment->in - p_shmfifo->shm_segment->out);
}


