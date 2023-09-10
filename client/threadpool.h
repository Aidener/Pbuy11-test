#ifndef PTHREAD_POOL_TEST
#define PTHREAD_POOL_TEST 
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include<stdatomic.h>

#define CORETHREAD_TIMEOUT_WAIT 1
#define CORETHREAD_TIMEOUT_EXIT 2

typedef struct {
    void (*task_function)(void*);
    void* arg;                                          
} Task;


typedef struct {
    pthread_t* threads;
    Task *queue;
    unsigned int  front, rear;
    unsigned int size;                              //当前任务队列大小
    unsigned  int queue_size;                  //预设任务队列大小
     unsigned int threads_count; 
    atomic_uint free_thread_count;        //当前无任务线程数           
    unsigned int max_threads;                //最大线程数
    unsigned int max_live_threads;        //最大核心线程数（最大存活线程数）
    unsigned int  max_free_time;            //最大空闲时间
    unsigned int coreThread_timeout_behaviour;                  //线程超时回收标志
    pthread_mutex_t mutex;
    pthread_cond_t cond_empty;
    pthread_cond_t cond_full;
    int shutdown;
} ThreadPool;

void thread_pool_wait_done(ThreadPool* pool) ;

void thread_pool_init(ThreadPool* pool,unsigned int  argc_max_threads,unsigned int argc_max_live_threads,
                                            unsigned int argc_max_free_time,unsigned int argc_coreThread_timeout_behaviour,
                                            unsigned int argc_queue_size);

int thread_pool_add_task(ThreadPool* pool, void (*task_function)(void*), void* arg) ;

void* thread_pool_worker(ThreadPool* pool);
//销毁线程池
void thread_pool_shutdown(ThreadPool* pool);                                                   

unsigned int getCount_of_FreeThreads(ThreadPool *pool);

unsigned int getCount_of_Threads(ThreadPool *pool);

unsigned int getCount_of_Task(ThreadPool *pool);
#endif // !PTHREAD_POOL_TEST
