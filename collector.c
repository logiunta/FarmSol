#define _POSIX_C_SOURCE 200112L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <collector.h>
#include <sys/un.h>
#include <signal.h>
#include <check_errors.h>
#include <sys/socket.h>
#include "socket_info.h"
#include <unistd.h>
#include "queue_utils.h"
#include <sys/mman.h>
#include<sys/select.h>


extern int* exitCollector;

static queue* list = NULL;


static void maskSignals(sigset_t *set){
    int err;
   // struct sigaction sa;

    sigemptyset(set);
    SYSCALL(err,pthread_sigmask(SIG_SETMASK,set,NULL),"pthread_sigmask");

    sigaddset(set, SIGPIPE);
    sigaddset(set, SIGINT);
    sigaddset(set, SIGHUP);
    sigaddset(set, SIGTERM);
    sigaddset(set, SIGQUIT);
    sigaddset(set, SIGUSR1);
   
    SYSCALL(err,pthread_sigmask(SIG_BLOCK,set,NULL),"pthread_sigmask");
  
}


void runCollector(int pfd){
    int fd_socket;
    struct sockaddr_un address; 
    int n = 0,len,err;
    char* buff = NULL;
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path,SOCKNAME,UNIX_MAX_PATH);

    sigset_t set_mask;
    maskSignals(&set_mask);
 
 
    SYSCALL(fd_socket,socket(AF_UNIX,SOCK_STREAM,0),"socket");

    SYSCALL(err,connect(fd_socket,(struct sockaddr*)&address,sizeof(address)),"connect");

    initQueue(&list);
    fd_set set, readyset;
    int fd_num = 0,fd;
    FD_ZERO(&set);

    FD_SET(fd_socket,&set);
    if(fd_socket > fd_num) fd_num = fd_socket;

    FD_SET(pfd,&set);
    if(pfd > fd_num) fd_num = pfd;

    printf("COLLECTOR: collegato al server\n");

    while(!(*exitCollector)){
        readyset = set;
        
        if((err = select(fd_num + 1,&readyset,NULL,NULL,NULL))==-1){
            printf("Chiusura pulita in corso\n");
            fflush(stdout);
            break;
            
        }
        else{
           
            for(fd=0;fd<=fd_num;fd++){
                
                if(FD_ISSET(fd,&readyset)){
                    
                    if(fd == fd_socket){ //pronta socket, leggo file dai workers
                        SYSCALL(n,read(fd_socket,&len,sizeof(int)),"read len");
                        if(n != 0){
                            buff = malloc(sizeof(char)*len+1);
                            CHECKNULL(buff,"malloc");
                            
                            if((n = read(fd_socket,buff,len)) == -1){
                                free(buff);
                                break;
                            }
                    
                            buff[len] = '\0';
                    
                            enqueueBack(&list,buff,-1);
                            free(buff);
                        }

                        
                    }

                    if(fd == pfd){ //pronta pipe, leggo notifica dal master
                        printf("\t\tPIPE PRONTA\n");

                        SYSCALL(n,read(pfd,&len,sizeof(int)),"read len from pipe");

                        if(n == 0){
                            printf("PIPE N = 0\n");
                            FD_CLR(pfd,&set);
                            if(fd == fd_num)
                                fd_num--;
                            break;
    
                        }

                    
                        buff = malloc(sizeof(char)*len+1);

                        CHECKNULL(buff,"malloc");
                        
                        if((n = read(pfd,buff,len)) == -1){
                            free(buff);
                            break;
                        }

                        buff[len] = '\0';
                        printf("COLLECTOR: leggo dalla pipe: %s\n",buff);
                        free(buff);
                        queueDisplay(list);


                        FD_CLR(pfd,&set);
                        if(fd == fd_num)
                            fd_num--;
                


                    }

                    else if(fd == pfd){
                        FD_SET(pfd,&set);
                        if(pfd > fd_num)
                            fd_num = pfd;

                    }
                }

            }
        }
    }


    queueDisplay(list);
    freeQueue(&list);
    printf("Collector: ho finito, mi chiudo\n");
    fflush(stdout);
    exit(0);
}