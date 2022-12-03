#define _POSIX_C_SOURCE 200112L
#include <stdlib.h>
#include <stdio.h>
#include "workers_pool.h"
#include "pthread_utils.h"
#include "signal.h"
#include <unistd.h>
#include <string.h>
#include "check_errors.h"
#include <errno.h>
#include <valid_file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "queue_utils.h"

threadinfo *threadsInfo = NULL;
pthread_t *workersPool = NULL;

pthread_mutex_t mutex_socket = PTHREAD_MUTEX_INITIALIZER;
extern queue* sharedQueue;
extern pthread_mutex_t mutex_queue;
extern pthread_cond_t cond_notEmpty;
extern pthread_cond_t cond_isFull;
extern int requestedExit;


static void executeTask(node* nodo,int tid){
    int fd_coll = nodo->fd;
    char* file = nodo->fileName;
    long n;
    FILE* in;
    long len;
    int sum = 0,err;

    //simulate work time for each execution

    int randomTime = (rand() % 3000) + 1000 ;
    printf("WOrker %d: eseguo il lavoro per %d(ms). . . \n",tid,randomTime);
    fflush(stdout);

    randomTime = randomTime/1000;
   
    sleep(randomTime);

  

    in = fopen(file,"rb");
    CHECKNULL(in,"fopen");

    fseek(in,0L,SEEK_END);
    len = ftell(in);
    fseek(in,0L,SEEK_SET);
 
    len = len/8;

    long buffIn[len];

    n = fread(buffIn,sizeof(long),len,in); 

    if(n != len){
        perror("fread");
        exit(EXIT_FAILURE);
    }   

    for(int i=0;i<len;i++){
        sum += (i*buffIn[i]);
        
    }

  //  printf("SOMMA per il file %s: %d\n",file,sum);
    Pthread_mutex_lock(&mutex_socket);
    printf("WORKER %d: acquisisco il lock per scrivere\n",tid);
    fflush(stdout);
    len = strlen(file);
    printf("Worker %d: consumato il file %s\n",tid,file);
    fflush(stdout);
    SYSCALL(err,write(fd_coll,&len,sizeof(int)),"write len");
    SYSCALL(err,write(fd_coll,file,len),"write buff");
    Pthread_mutex_unlock(&mutex_socket);
   

    fclose(in);
    
    freeSingleNode(&nodo);


}

void* task(void* args){
    int tid = ((threadinfo*)args)->tid;
    int _exit = 0;

    while(!_exit){
        Pthread_mutex_lock(&mutex_queue);
        
        if(queueLen(sharedQueue) == 0 && requestedExit){
            Pthread_mutex_unlock(&mutex_queue);
            Pthread_cond_signal(&cond_isFull);
            _exit = 1;
            
        }

        else{
            while(queueLen(sharedQueue) == 0 && !requestedExit){
                printf("WORKER %d in attesa\n",tid);
                Pthread_cond_wait(&cond_notEmpty,&mutex_queue);
                printf("Worker %d risvegliato\n",tid);
                fflush(stdout);

            }

            if(queueLen(sharedQueue) > 0){
                node* nodo = dequeueFront(&sharedQueue);            
                Pthread_mutex_unlock(&mutex_queue);
                Pthread_cond_signal(&cond_isFull);

                executeTask(nodo,tid);
            }

            else {
                Pthread_mutex_unlock(&mutex_queue);
                Pthread_cond_signal(&cond_isFull);
            }
        }
    
    }
        
   
    printf("Worker %d: esco dal pool e mi chiudo: requesteExit = %d\n",tid,requestedExit);
    pthread_exit(0);

        
}


