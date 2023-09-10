#include"threadpool.h"
#include<time.h>
void thread_pool_init(ThreadPool* pool,unsigned int  argc_max_threads,unsigned int argc_max_live_threads,
                                           unsigned int argc_max_free_time,unsigned int argc_coreThread_timeout_behaviour,
                                            unsigned int argc_queue_size) {
    pool->front = pool->rear = pool->size = 0;
    pool->max_threads=argc_max_threads;
    pool->max_live_threads=argc_max_live_threads;
    pool->max_free_time=argc_max_free_time;
    pool->coreThread_timeout_behaviour=argc_coreThread_timeout_behaviour;
    pool->queue_size=argc_queue_size;
    pool->shutdown = 0;                                                //初始化线程池参数

    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond_empty, NULL);
    pthread_cond_init(&pool->cond_full, NULL);               //初始化线程池条件变量以及互斥锁

    pool->threads=calloc(sizeof(pthread_t),argc_max_threads);
    pool->queue=calloc(sizeof(Task),argc_queue_size);                    //分配队列以及线程变量内存

    int i;
    pool->threads_count=argc_max_live_threads; 
    pool->free_thread_count=ATOMIC_VAR_INIT(argc_max_live_threads);               //原子变量赋值
    for (i = 0; i < pool->max_live_threads; ++i)
        pthread_create(&(pool->threads[i]), NULL, (void*(*)(void*))thread_pool_worker, pool);         //创建核心线程     
}

int thread_pool_add_task(ThreadPool* pool, void (*task_function)(void*), void* arg) {

    pthread_mutex_lock(&pool->mutex);
    while (pool->size == pool->queue_size) {                                       //主线程单生产者环境，等待队列空余
        pthread_cond_wait(&pool->cond_empty, &pool->mutex);
    }
    
    if(atomic_load(& pool->free_thread_count)<1 && pool->threads_count<pool->max_threads){  //没有空闲线程，创建新线程
         pthread_create(&pool->threads[pool->threads_count],NULL,(void*(*)(void*))thread_pool_worker,pool);
         pool->threads_count++;
         atomic_fetch_add(&pool->free_thread_count,1); 
    }
    pool->queue[pool->rear].task_function = task_function;
    pool->queue[pool->rear].arg = arg;
    pool->rear = (pool->rear + 1) % pool->queue_size;
    pool->size++;

    pthread_cond_signal(&pool->cond_full);                                 
    pthread_mutex_unlock(&pool->mutex);                                    //通知消费者消费并释放锁

    return 1;
}

void* thread_pool_worker(ThreadPool* pool) {
    struct  timespec wait_time;
    while (1) {
        pthread_mutex_lock(&pool->mutex); 
        while (pool->size == 0 && !pool->shutdown) {
           clock_gettime(CLOCK_REALTIME,&wait_time);
            wait_time.tv_sec+=pool->max_free_time;
            int result=pthread_cond_timedwait(&pool->cond_full, &pool->mutex,&wait_time);                            //等待生产者生产
            if(result!=0  ){                                                                                                                                                 //等待超时，回收线程
                    if(pool->coreThread_timeout_behaviour==CORETHREAD_TIMEOUT_WAIT
                       && pool->max_live_threads >= pool->threads_count)
                             continue;
                    if(pthread_self()!=pool->threads[pool->threads_count-1]){          //交换当前线程为队列最后一个
                            int i=0;
                            while(pthread_self()!=pool->threads[i])
                            i++;
                            pthread_t tem=pool->threads[pool->threads_count-1];               
                            pool->threads[pool->threads_count-1]=pthread_self();
                            pool->threads[i]=tem;
                    }
                    pool->threads_count--;
                    pthread_mutex_unlock(&pool->mutex);                                //回收线程
                    atomic_fetch_sub(&pool->free_thread_count,1);
                    pthread_exit(NULL);
            }
        }

        if (pool->shutdown) {
            pthread_cond_signal(&pool->cond_full);             //通知下一个等待的线程，迅速结束，线程池即将销毁
            pthread_mutex_unlock(&pool->mutex);
            pthread_exit(NULL);
        }

        void (*task_function)(void*) = pool->queue[pool->front].task_function;
        void* arg = pool->queue[pool->front].arg;
        pool->front = (pool->front + 1) % pool->queue_size;
        pool->size--;

        pthread_cond_signal(&pool->cond_empty);                    //提醒生产者，可以投递
        if(pool->size>0)
              pthread_cond_signal(&pool->cond_full);          //通知其他消费线程，我已经完成任务取出，当前任务队列可以继续消费
        pthread_mutex_unlock(&pool->mutex);
        atomic_fetch_sub(&pool->free_thread_count,1);             //执行任务，当前线程忙碌
                 //使用原子变量，降低锁的开销
        task_function(arg);                                                       //执行任务
        atomic_fetch_add(&pool->free_thread_count,1);          //执行完成，当前线程空闲
    }
}

void thread_pool_wait_done(ThreadPool* pool) {
    pthread_mutex_lock(&pool->mutex);
    while (pool->size > 0) {
        // 等待条件变量，直到线程池中的任务全部执行完毕
        pthread_cond_wait(&pool->cond_empty, &pool->mutex);
    }
    pthread_mutex_unlock(&pool->mutex);
}

void thread_pool_shutdown(ThreadPool* pool) {
    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = 1;
    pthread_mutex_unlock(&pool->mutex);

    pthread_cond_broadcast(&pool->cond_full);

    int i;
    for (i = 0; i < pool->max_threads; ++i)
        pthread_join(pool->threads[i], NULL);
    
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond_empty);
    pthread_cond_destroy(&pool->cond_full);
    free(pool->threads);
    free(pool->queue);                                                  //释放动态内存
}


unsigned int getCount_of_FreeThreads(ThreadPool *pool){
    return atomic_load(&pool->free_thread_count);
}

unsigned int getCount_of_Threads(ThreadPool *pool){
    return pool->threads_count;
}

unsigned int getCount_of_Task(ThreadPool *pool){
   return pool->size;
}
