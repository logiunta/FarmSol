#define _GNU_SOURCE
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
#include "master.h"
#include "check_errors.h"
#include "queue_utils.h"
#include <pthread.h>
#include <signal.h>
#include <collector.h>
#include <socket_info.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>

// volatile sig_atomic_t* printValues;
int *exitCollector;




int main(int argc, char *argv[])
{   
    int fd_socket,err;
    struct sockaddr_un sa;
    pid_t pid;
    int pfds[2];
    int pfd;

    cleanup();
    atexit(cleanup);
 
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path,SOCKNAME,UNIX_MAX_PATH);
        
    SYSCALL(fd_socket,socket(AF_UNIX,SOCK_STREAM,0),"socket");

    SYSCALL(err,bind(fd_socket,(struct sockaddr*)&sa,sizeof(sa)),"bind");

    SYSCALL(err,listen(fd_socket,SOMAXCONN),"listen");

 
    exitCollector = (int*)mmap(NULL,sizeof(int),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS ,-1, 0);

      if (*exitCollector == -1) {
        perror("mmap");
        return -1;
    }

    *exitCollector = 0;
    SYSCALL(err,pipe(pfds),"pfd pipe");

    SYSCALL(pid,fork(),"fork");

    //parent -> Master
    if(pid != 0){
        //il padre chiude la lettura dato che deve solo scrivere al figlio
        close(pfds[0]);
        pfd = pfds[1];
        runMaster(argc,argv,pid,fd_socket,pfd);
     
    }

    //child -> collector
    else{
        //il figlio chiude la scrittura dato che deve solo leggere dal padre
        close(pfds[1]);
        pfd = pfds[0];
        runCollector(pfd);

    }


}




