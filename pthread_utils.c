#include <errno.h>
#include <stdio.h>
#include "pthread_utils.h"


void Pthread_mutex_lock(pthread_mutex_t *mutex){
    int err;
    if((err = pthread_mutex_lock(mutex)) != 0){
        errno = err;
        perror("lock");
        pthread_exit((void*)&errno);
    }

}


void Pthread_mutex_unlock(pthread_mutex_t *mutex){
    int err;
    if((err = pthread_mutex_unlock(mutex)) != 0){
        errno = err;
        perror("unlock");
        pthread_exit((void*)&errno);
    }

}


void Pthread_cond_signal(pthread_cond_t *cond){
    int err;
    if((err = pthread_cond_signal(cond))!=0){
        errno = err;
        perror("signal");
        pthread_exit((void*)&errno);
    }
}

void Pthread_cond_wait(pthread_cond_t *cond,pthread_mutex_t *mutex){
    int err;
    if((err = pthread_cond_wait(cond,mutex))!=0){
        errno = err;
        perror("wait");
        pthread_exit((void*)&errno);
    }
}

