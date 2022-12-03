#ifndef MASTER
#define MASTER

#define NTHREADS 4
#define QUEUESIZE 8


void cleanup();
void runMaster(int argc,char* argv[],int pid,int fd_socket,int pfd);


#endif