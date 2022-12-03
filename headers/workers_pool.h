#ifndef WORKERS_POOL
#define WORKERS_POOL

typedef struct threadInfo{
    int tid;
}threadinfo;


void* task(void* args);


#endif