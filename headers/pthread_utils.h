#ifndef PTHREAD_UTILS
#define PTHREAD_UTILS

#include <pthread.h>


 void Pthread_mutex_lock(pthread_mutex_t *mutex);

 void Pthread_mutex_unlock(pthread_mutex_t *mutex);

 void Pthread_cond_signal(pthread_cond_t *cond);

 void Pthread_cond_wait(pthread_cond_t *cond,pthread_mutex_t *mutex);


#endif
