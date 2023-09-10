#ifndef RESOURCE_H
#define RESOURCE_H
#include<pthread.h>
#include<errno.h>
#define NUM_RESOURCES 100
#define NUM_SEGMENTS 100

typedef struct {
    int resource[NUM_RESOURCES];
    pthread_mutex_t lock;
    int rem;
} Segment;

extern  Segment segments[NUM_SEGMENTS];

void init_segments() ;

int access_resource(int segment_index, int* resource_index);

void destory_segment();

#endif // !RESOURCE_H