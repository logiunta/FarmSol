#define _POSIX_C_SOURCE 200112L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "collector.h"
#include <sys/un.h>
#include <signal.h>
#include "check_errors.h"
#include <sys/socket.h>
#include "socket_info.h"
#include <unistd.h>
#include <sys/select.h>
#include "results_utils.h"


static resFile* results = NULL;
static char* exitString = "exit";
static char* displayString = "display";


static void maskSignals(sigset_t *set){
    int err;
    struct sigaction s;
    sigfillset(set);
    SYSCALL(err,pthread_sigmask(SIG_SETMASK,set,NULL),"pthread_sigmask");
    memset(&s,0,sizeof(s));
    s.sa_handler = SIG_IGN;
    SYSCALL(err, sigaction(SIGPIPE,&s,NULL),"Sigaction");

    sigemptyset(set);
    SYSCALL(err,pthread_sigmask(SIG_SETMASK,set,NULL),"pthread_sigmask");

    sigaddset(set, SIGINT);
    sigaddset(set, SIGHUP);
    sigaddset(set, SIGTERM);
    sigaddset(set, SIGQUIT);
    sigaddset(set, SIGUSR1);
    sigaddset(set, SIGALRM);
   
    SYSCALL(err,pthread_sigmask(SIG_BLOCK,set,NULL),"pthread_sigmask");
  
}

//Using for testing results
// static void writeOnFile(resFile* results,int size){

//     FILE* out = fopen("resultsCollector.txt","w");
   
//     CHECKNULL(out,"fopen");
//     fprintf(out,"COLLECTOR:ARRIVATI %d file\n",size);
//     for(int i =0;i<size;i++){
//        fprintf(out,"%ld %s\n",results[i].sum,results[i].fileName);

//     }
    
  

// }


void runCollector(int pfd){
    int fd_socket;
    struct sockaddr_un address; 
    int n = 0,len,err,count = 0, ok  = 1;
    long sum;
    char* buff = NULL;
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path,SOCKET,UNIX_MAX_PATH);
    int _exit = 0;
    sigset_t set_mask;
    maskSignals(&set_mask);
 
 
    SYSCALL(fd_socket,socket(AF_UNIX,SOCK_STREAM,0),"socket");

    SYSCALL(err,connect(fd_socket,(struct sockaddr*)&address,sizeof(address)),"connect");
  
    fd_set set, readyset;
    int fd_num = 0,fd;
    FD_ZERO(&set);

    FD_SET(fd_socket,&set);
    if(fd_socket > fd_num) fd_num = fd_socket;

    FD_SET(pfd,&set);
    if(pfd > fd_num) fd_num = pfd;


    // FILE* fileRes = fopen("fileArrivedFromSocket.txt","w");
   
    // CHECKNULL(fileRes,"fopen");
    
    printf("COLLECTOR: connected\n");

    //ok mi serve per sapere se ci sono ancora dati da leggere dalla socket
    while(!_exit || ok){
        readyset = set;
        
        if((err = select(fd_num + 1,&readyset,NULL,NULL,NULL))==-1){
            break;
        }
        else{
           
            for(fd=0;fd<=fd_num;fd++){
                
                if(FD_ISSET(fd,&readyset)){
                                
                    if(fd == fd_socket){ //pronta socket, leggo file dai workers
                        SYSCALL(n,read(fd_socket,&sum,sizeof(long)),"read sum");
                        if(n != 0){
                            SYSCALL(n,read(fd_socket,&len,sizeof(int)),"read len");
                            
                            if(n != 0){
                                buff = malloc(sizeof(char)*len+1);
                                CHECKNULL(buff,"malloc");
                                
                                if((n = read(fd_socket,buff,len)) == -1){
                                    free(buff);
                                    break;
                                }

                                buff[len] = '\0';

                              //  fprintf(fileRes,"%ld %s\n",sum,buff);
                                addResult(&results,sum,buff,count);
                                count++;
                                free(buff);
                               
                            }
                        }
                        //la read ritorna 0, non c'è più niente da leggere dal socket
                        else {
                            ok = 0;
                            break;
                        }

                    }

                    if(fd == pfd){ //pronta pipe, leggo notifica dal master
                        SYSCALL(n,read(pfd,&len,sizeof(int)),"read len from pipe");
                        if(n == 0){
                            FD_CLR(pfd,&set);
                            if(fd == fd_num)
                                fd_num--;
                            ok = 0;
                            break;
    
                        }

                        buff = malloc(sizeof(char)*len+1);
                        CHECKNULL(buff,"malloc");

                        if((n = read(pfd,buff,len)) == -1){
                            free(buff);
                            break;
                        }

                        buff[len] = '\0';
                        
                        if(strncmp(buff,displayString,len) == 0){
                            sortResults(&results,count);
                            displayResults(results,count);
                            free(buff);
                        }

                        else if (strncmp(buff,exitString,len) == 0){
                            _exit = 1;
                            free(buff);
                            break;
                        }

                    }

                }
               

            }
        }
    }

     

   // fclose(fileRes);
    sortResults(&results,count);
    //writeOnFile(results,count);
    displayResults(results,count);
    freeResults(&results,count);
    printf("Collector: closing\n");
    fflush(stdout);
  
    exit(0);
}