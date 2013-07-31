/**
 * threadpool.c
 *
 * This file will contains implementation of threadpool.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/time.h>
#include "threadpool.h"

// _threadpool is the internal threadpool structure that is
// cast to type "threadpool" before it given out to callers

typedef struct _queue {
  dispatch_fn my_func;
	void* arg1;
	struct _queue * next;
}queue;



typedef struct _threadpool_st {
    	pthread_t *tid;
	pthread_mutex_t mutextask;
  	pthread_cond_t task;
	pthread_cond_t availableThread;
	pthread_mutex_t mutexThread;
	int count;
	int activeThreadCnt;
	queue   *jobs;

} _threadpool;

static int queue_count = 0;
int overallCount = 0;
struct timeval tv;
time_t curtime;

void wait(void *processthreadpool);

threadpool create_threadpool(int num_threads_in_pool) {

  _threadpool *pool;
  

  // sanity check the argument
  if ((num_threads_in_pool <= 0) || (num_threads_in_pool > MAXT_IN_POOL))
    return NULL;

  pool = (_threadpool *) malloc(sizeof(_threadpool));
  if (pool == NULL) {
    fprintf(stderr, "Out of memory creating a new threadpool!\n");
    return NULL;
  }
  pool->tid = (pthread_t*)malloc((sizeof(pthread_t)*num_threads_in_pool));
  // add your code here to initialize the newly created threadpool
  pool->count = num_threads_in_pool;
  pool->activeThreadCnt = num_threads_in_pool;

  pthread_mutex_init(&pool->mutextask,NULL);
   int rc, t;
   for(t=0; t<num_threads_in_pool; t++){
      printf("In main: creating thread %d\n", t);
	 
      rc = pthread_create(&pool->tid[t], NULL, wait, (void *)pool);
      if (rc){
         printf("ERROR; return code from pthread_create() is %d\n", rc);
         exit(-1);
      }
   }

  return (threadpool) pool;
}


void dispatch(threadpool from_me, dispatch_fn dispatch_to_here,
	      void *arg) {

  _threadpool *pool = (_threadpool *) from_me;
  queue * new_elem ;
  queue *my_queue;
  overallCount ++;
  if (overallCount%50 == 0)
  {
	gettimeofday(&tv, NULL); 
  	curtime=tv.tv_sec;
  	printf("%ld\n",tv.tv_usec);
  }

  pthread_mutex_lock(&pool->mutextask);
  my_queue = pool->jobs;

  // add your code here to dispatch a thread
	if(!my_queue)
	{
		my_queue = (queue *)malloc(sizeof(queue));
		my_queue->my_func = dispatch_to_here;
		my_queue->arg1 = arg;
		my_queue->next = 0;
		pool->jobs = my_queue;
		queue_count++;
	}
	else {
		while(my_queue->next != NULL)
		{
			my_queue = my_queue->next;
		}
		new_elem = (queue *)malloc(sizeof(queue));
		new_elem->my_func = dispatch_to_here;
		new_elem->arg1 = arg;
		new_elem->next = 0;
		my_queue->next = new_elem;
		queue_count++;
	
     }
     if(pool->activeThreadCnt > 0)
	pthread_cond_signal(&pool->task);
	
     else
	{
		pthread_cond_wait(&pool->availableThread, &pool->mutextask);	
	}
	
  pthread_mutex_unlock(&pool->mutextask);

}

void destroy_threadpool(threadpool destroyme) {
  _threadpool *pool = (_threadpool *) destroyme;

  // add your code here to kill a threadpool
	

}

void wait(void *processthreadpool)
{
	 _threadpool *pool = (_threadpool *) processthreadpool;
	queue *temp;
	while(1)
	{

		pthread_mutex_lock(&pool->mutextask);
		pthread_cond_wait(&pool->task, &pool->mutextask);		
		//printf("%d\n",pthread_self());
		temp = pool->jobs;
		queue_count--;
		pool->activeThreadCnt--;
		pool->jobs = pool->jobs->next;
		pthread_mutex_unlock(&pool->mutextask);
		
		temp->my_func(temp->arg1);
		pool->activeThreadCnt++;
		pthread_cond_signal(&pool->availableThread);
		free(temp->arg1);
		free(temp);
				
	}	
}
