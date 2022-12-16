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
#include <sys/time.h>

static sigset_t set;
static sigset_t setMaster;
static pthread_t tidMaster;

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

void handlerAlarm(int sig){
    // write(1,"MASTER: timer scaduto, carico sulla coda\n",42);
    return;

}


static void setMasterMask(){
    int err;
    struct sigaction s;
    sigfillset(&setMaster);
    SYSCALL(err,pthread_sigmask(SIG_SETMASK,&setMaster,NULL),"pthread_sigmask");
    memset(&s,0,sizeof(s));

    s.sa_handler = handlerAlarm;
    SYSCALL(err, sigaction(SIGALRM,&s,NULL),"Sigaction");
    s.sa_handler = SIG_IGN;
    SYSCALL(err, sigaction(SIGPIPE,&s,NULL),"Sigaction");
    
    sigemptyset(&setMaster);
    SYSCALL(err,pthread_sigmask(SIG_SETMASK,&setMaster,NULL),"pthread_sigmask");
    sigaddset(&setMaster, SIGUSR1);
    sigaddset(&setMaster, SIGINT);
    sigaddset(&setMaster, SIGHUP);
    sigaddset(&setMaster, SIGTERM);
    sigaddset(&setMaster, SIGQUIT);
    SYSCALL(err,pthread_sigmask(SIG_BLOCK,&setMaster,NULL),"pthread_sigmask");
    
}

static void setHandlerMask(){
    int err;
    struct sigaction s;
    sigfillset(&setMaster);
    SYSCALL(err,pthread_sigmask(SIG_SETMASK,&setMaster,NULL),"pthread_sigmask");
    memset(&s,0,sizeof(s));
    
    s.sa_handler = SIG_IGN;
    SYSCALL(err, sigaction(SIGPIPE,&s,NULL),"Sigaction");
    sigemptyset(&set);
    SYSCALL(err,pthread_sigmask(SIG_SETMASK,&set,NULL),"pthread_sigmask");

    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGQUIT);
   
    SYSCALL(err,pthread_sigmask(SIG_BLOCK,&set,NULL),"pthread_sigmask");
}


void cleanup(){
    unlink(SOCKNAME);
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
    int err;

    setHandlerMask();
    printf("HANDLER: waiting for some signal. . .\n");
    while(!requestedExit){
        sigwait(&set,&sig);

        switch (sig)
        {
            case SIGQUIT:
            case SIGHUP:
            case SIGTERM:
            case SIGINT:
                printf("HANDLER: closure request received\n");
                SYSCALL(err,pthread_kill(tidMaster,SIGALRM),"kill");
                requestedExit = 1;
                break;
          
            //caso in cui il master deve inviare una notifica di stampa al collector
            case SIGUSR1:
                printf("HANDLER: notify to collector\n");
                fflush(stdout);
                notifyDisplayToCollector(*pfd);
                break;
            
            case SIGALRM:
                break;
            default:
                break;
        }
    }   

    printf("HANDLER: closing\n");
    pthread_exit(0);
}



static void addInSharedQueue(int fd){
    node* nodoBin = (dequeueFront(&listBin));
    char* file = nodoBin->fileName;
    Pthread_mutex_lock(&mutex_queue);
    while(queueLen(sharedQueue) >= queueDim){
        printf("MASTER: queue is full, waiting for it to release\n");
        fflush(stdout);
        Pthread_cond_wait(&cond_isFull,&mutex_queue);
        
    }

    enqueueBack(&sharedQueue,file,fd); 
    freeSingleNode(&nodoBin); 
    Pthread_cond_signal(&cond_notEmpty);   
    Pthread_mutex_unlock(&mutex_queue);
   
  
}


void runMaster(int argc,char* argv[],int pid,int fd_socket,int pfd){
    int n = 0, q = 0, res,err;
    long t = 0;
    int status;
    int _exit = 0;
    pthread_t signalHandler;
    struct itimerval timer;
    tidMaster = pthread_self();

    int fd_col;

    setMasterMask();

    initQueue(&listBin);

    res = parseArguments(argc,argv,&n,&q,&t,&listBin);

    if(queueLen(listBin) == 0 || res == -1){
        fprintf(stderr,"MASTER: error reading arguments\n");
        freeQueue(&listBin);
        notifyCloseToCollector(pfd);
        cleanup();
        atexit(cleanup);
        exit(EXIT_FAILURE);
      
    }
 
    if (q > 0)
        queueDim = q;

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
        if(t != 0){
            timer.it_interval.tv_sec = 0;
            timer.it_interval.tv_usec = 0; /*no reload*/
            if(t >= 1000){
                timer.it_value.tv_sec = (t/1000);
                timer.it_value.tv_usec = 0;

            }
            else{
                timer.it_value.tv_sec = 0;
                timer.it_value.tv_usec = (t*100);
            }

            SYSCALL(err,setitimer(ITIMER_REAL,&timer,NULL),"setitimer"); //when timer expires a SIGALARM is raised and wake up from pause
            pause();
            
        }

        if(!requestedExit && queueLen(listBin) > 0){
            printf("Master:add in queue\n");
            addInSharedQueue(fd_col);
        }

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

    printf("MASTER: waiting for collector\n");
    notifyCloseToCollector(pfd);
    
    //dopo il join dei workers posso chiudere fd_col
    close(fd_col);
    free(threadsInfo);
    free(workersPool);

    freeQueue(&listBin);

    Pthread_mutex_lock(&mutex_queue);
    freeQueue(&sharedQueue);
    Pthread_mutex_unlock(&mutex_queue);

    SYSCALL(err,waitpid(pid, &status,WUNTRACED),"waitpid");
    print_status(pid,status);

    //una volta che il collector è chiuso il master può chiudere il thread signal handler
    SYSCALL(err,pthread_kill(signalHandler,SIGALRM),"kill");

    
    if(pthread_join(signalHandler,NULL) != 0){
        perror("join");
        exit(EXIT_FAILURE);
    }


    close(fd_socket);
    cleanup();
    atexit(cleanup);


    exit(0);

   

}