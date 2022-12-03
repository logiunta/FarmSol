#ifndef CHECK_ERRORS
#define CHECK_ERRORS


#define CHECKNULL(M,b) if(M == NULL) { \
    perror(b); \
    exit(EXIT_FAILURE); \
}

#define SYSCALL(r,c,e) if((r=c)==-1) { \
    perror(e); \
    exit(errno); \
}     

#endif