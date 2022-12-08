#define _POSIX_C_SOURCE 200112L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <ctype.h>
#include <sys/un.h>
#include <signal.h>
#include "queue_utils.h"
#include "pthread_utils.h"
#include <sys/select.h>
#include "master.h"
#include "check_errors.h"
#include "parse_arguments.h"
#include "workers_pool.h"
#include "socket_info.h"
#include "collector.h"

static sigset_t set;
static queue* listBin = NULL;
static int nthreads = NTHREADS;
static int queueDim = QUEUESIZE;

extern threadinfo *threadsInfo;
extern pthread_t *workersPool;

queue* sharedQueue = NULL;
pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_pipe = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_notEmpty = PTHREAD_COND_INITIALIZER; 
pthread_cond_t cond_isFull = PTHREAD_COND_INITIALIZER;

volatile int requestedExit = 0;

static void print_status(int pid,int status){
    printf("\nPID of the child process that was just created:  %d.\n", pid);

    if(WIFEXITED(status))
    {
        printf("PID %d exited normally.  Exit number:  %d\n", pid, WEXITSTATUS(status));
    }
    else if(WIFSTOPPED(status))
    {
        printf("PID %d was stopped by %d\n", pid, WSTOPSIG(status));
    }
    else if(WIFSIGNALED(status))
    {
        printf("PID %d exited due to signal %d\n.", pid, WTERMSIG(status));
    }
    
    fflush(stdout);
}


static void notifyCloseToCollector(int pfd){
    int err;
    char* message = "exit";
    int len = strlen(message);
    Pthread_mutex_lock(&mutex_pipe);
    SYSCALL(err,write(pfd,&len,sizeof(int)),"write len");
    SYSCALL(err,write(pfd,message,len),"write buff");
    Pthread_mutex_unlock(&mutex_pipe);
  
}

static void notifyDisplayToCollector(int pfd){
    int err;
    char* message = "display";
    int len = strlen(message);
    Pthread_mutex_lock(&mutex_pipe);
    SYSCALL(err,write(pfd,&len,sizeof(int)),"write len");
    SYSCALL(err,write(pfd,message,len),"write buff");
    Pthread_mutex_unlock(&mutex_pipe);
  

}

static void* handler(void* args){
    int* pfd = (int*)args;
    int sig = 0;
   
    while(!requestedExit){
        sigwait(&set,&sig);

        switch (sig)
        {
            case SIGQUIT:
            case SIGHUP:
            case SIGTERM:
            case SIGPIPE:
            case SIGINT:
                printf("SEGNALE CATTURATO\n");
                requestedExit = 1;
                break;
          
            //caso in cui il master deve inviare una notifica di stampa al collector
            case SIGUSR1:
                notifyDisplayToCollector(*pfd);
                break;
            
            case SIGALRM:
                break;
            default:
                break;
        }
    }   

  
    pthread_exit(0);
}



static void setMask(){
    int err;

    sigemptyset(&set);
    SYSCALL(err,pthread_sigmask(SIG_SETMASK,&set,NULL),"pthread_sigmask");

    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGPIPE);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGQUIT);
   
    SYSCALL(err,pthread_sigmask(SIG_BLOCK,&set,NULL),"pthread_sigmask");
}


void cleanup(){
    unlink(SOCKNAME);
}


static void addInSharedQueue(int fd){
    node* nodoBin = (dequeueFront(&listBin));
    char* file = nodoBin->fileName;
    Pthread_mutex_lock(&mutex_queue);
    while(queueLen(sharedQueue) >= queueDim){
        Pthread_cond_wait(&cond_isFull,&mutex_queue);
        
    }

    enqueueBack(&sharedQueue,file,fd); 
    freeSingleNode(&nodoBin); 
    Pthread_mutex_unlock(&mutex_queue);
    Pthread_cond_signal(&cond_notEmpty);   
  
}


void runMaster(int argc,char* argv[],int pid,int fd_socket,int pfd){
    int n = 0, q = 0, res,err;
    long t = 0;
    int status;
    int _exit = 0;
    pthread_t signalHandler;

    int fd_col;

    setMask();

    initQueue(&listBin);

    res = parseArguments(argc,argv,&n,&q,&t,&listBin);

    if(queueLen(listBin) == 0){
        fprintf(stderr,"No correct files found\n");
        freeQueue(&listBin);
        notifyCloseToCollector(pfd);
        exit(EXIT_FAILURE);

    }
 
    if(res == -1){
        freeQueue(&listBin);
        notifyCloseToCollector(pfd);
        exit(EXIT_FAILURE);
    }


    if(n > 0)
        nthreads = n;

     //creo il pool di threads
    workersPool = malloc(sizeof(pthread_t)*nthreads);
    threadsInfo = malloc(sizeof(threadinfo)*nthreads);

    SYSCALL(err,pthread_create(&signalHandler,NULL,handler,(void*)&pfd),"pthread_create");

  
    for(int i=0;i<nthreads;i++){
        threadsInfo[i].tid = i;
        SYSCALL(err,pthread_create(&workersPool[i],NULL,task,(void*)&threadsInfo[i]),"pthread_create");
        
    }


    initQueue(&sharedQueue);

    SYSCALL(fd_col,accept(fd_socket,NULL,NULL),"accept");
    while(!_exit){
        if(t != 0)
            sleep((t/1000));

        if(!requestedExit && queueLen(listBin) > 0)
            addInSharedQueue(fd_col);

        else _exit = 1;
        
    }
    
        
    
    //il master comunica ai workers di chiudere
    if(!requestedExit)
        requestedExit = 1;
    
    pthread_cond_broadcast(&cond_notEmpty);

  
    for(int i=0;i<nthreads;i++){
        if(pthread_join(workersPool[i],NULL) != 0){
            perror("join");
            exit(EXIT_FAILURE);
        }
    }


    //master comunica al collector di chiudere  
    notifyCloseToCollector(pfd);

    //dopo il join so che nessun worker sta più usando fd_col
    close(fd_col);
    free(threadsInfo);
    free(workersPool);

    freeQueue(&listBin);

    Pthread_mutex_lock(&mutex_queue);
    freeQueue(&sharedQueue);
    Pthread_mutex_unlock(&mutex_queue);

    SYSCALL(err,waitpid(pid, &status,WUNTRACED),"waitpid");
    print_status(pid,status);


    close(fd_socket);
 
    //una volta che il collector è chiuso il master può chiudere il thread signal handler
    SYSCALL(err,pthread_kill(signalHandler,SIGALRM),"kill");

    if(pthread_join(signalHandler,NULL) != 0){
            perror("join");
            exit(EXIT_FAILURE);
    }


    cleanup();
    atexit(cleanup);


    exit(0);

   

}