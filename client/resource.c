#include"resource.h"

Segment segments[NUM_SEGMENTS];

void init_segments() {
    int i=0,j=0;
    for (i = 0; i < NUM_SEGMENTS; i++) {
        for (j = 0; j < NUM_RESOURCES; j++) {
            segments[i].resource[j] = i*100+j;
        }
        pthread_mutex_init(&segments[i].lock, NULL);
        segments[i].rem=99;                                                   //指向队列最后一件
    }
}

//尝试获得资源
int  access_resource(int segment_index, int* resource) {
    int busy=pthread_mutex_trylock(&segments[segment_index].lock);           //尝试加锁段                
    if(busy==EBUSY){                                                                            //当前段已经被其他消费者加锁
        return -1;
    }
    // 访问资源
    if(segments[segment_index].rem>-1)                                      //检查段中资源是否还有库存
        *resource=segments[segment_index].resource[ segments[segment_index].rem-- ];
    else{
         pthread_mutex_unlock(&segments[segment_index].lock);                //资源已经抢完
        return -1;
    }
      pthread_mutex_unlock(&segments[segment_index].lock);
    return 0;
}

void destory_segment(){
    int i;
    for(i=0;i<NUM_SEGMENTS;i++){
        pthread_mutex_destroy(&segments[i].lock);
    }
}