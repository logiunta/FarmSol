#ifndef QUEUE_UTILS
#define QUEUE_UTILS


typedef struct node
{
    int fd;
    char *fileName;
    struct node *next;
    
}node;

typedef struct Queue{
    int size;
    node* head;
}queue;


void initQueue(queue** q);
int queueLen(queue *q);
void enqueueBack(queue **q, char *val, int fd);
void queueDisplay(queue *q);
void freeQueue(queue **q);
node* dequeueFront(queue** q);
void freeSingleNode(node** node);


#endif

