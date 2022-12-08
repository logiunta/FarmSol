#ifndef RESULTS_UTILS
#define RESULTS_UTILS



typedef struct ResFile{
    long sum;
    char* fileName;
    struct ResFile *next;

}resFile;

typedef struct ResQueue{
    int size;
    resFile* head;
}resQueue;

void initResQueue(resQueue** q);
int resQueueLen(resQueue *q);
void resEnqueueFront(resQueue **q, char *val, long sum);
void resEnqueueBack(resQueue **q, char *val, long sum);
void freeResQueue(resQueue **q);
resFile* resDequeueFront(resQueue** q);
void freeNode(resFile** node);
void queueResultsDisplay(resQueue* q);
void sortQueue(resQueue** q);


#endif

