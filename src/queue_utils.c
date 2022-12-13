#include <stdio.h>
#include <stdlib.h>
#include "queue_utils.h"
#include "check_errors.h"
#include <string.h>


void initQueue(queue** q){
    *q = malloc(sizeof(queue));
    CHECKNULL(*q,"malloc");
    (*q)->head = NULL;
    (*q)->size = 0;
}


node* dequeueFront(queue** q){
  
    if((*q)->head == NULL){
        printf("lista vuota\n");
        return NULL;
    }
    
    node* tmp = (*q)->head;

    (*q)->head = ((*q)->head)->next;
    (*q)->size--;
   
    return tmp;
}

void enqueueBack(queue** q, char *val, int fd){
    node *tmp = malloc(sizeof(node));
    CHECKNULL(tmp,"malloc");
    node *currHead = (*q)->head;
    int len;
    if(val != 0){
            
        len = strlen(val);
        tmp->fileName = malloc(sizeof(char)*len+1);
        CHECKNULL(tmp->fileName,"malloc");
        strncpy(tmp->fileName,val,len);
        tmp->fileName[len] = '\0';
        tmp->fd = fd;
        tmp->next = NULL;
    
        
        if((*q)->head==NULL){
            (*q)->head = tmp; 
            
        }
        else{
            while(currHead->next!=NULL){
                currHead = currHead->next;
            }
            currHead->next = tmp;
        }

        (*q)->size++;
    }

}

int queueLen(queue* q){
    if(q == NULL)
        return 0;
    return q->size;
  
}

void queueDisplayWithFd(queue* q){
    node *curr = q->head;
    while(curr != NULL){
        printf("(%d): %s " , curr->fd, curr->fileName);
          if(curr->next != NULL)
            printf("-> ");
        curr = curr->next;
    }
    printf("\n");
}


void queueDisplay(queue* q){
    node *curr = q->head;
    while(curr != NULL){
        printf("%s ",curr->fileName);
        if(curr->next != NULL)
            printf("-> ");

        curr = curr->next;
    }
    printf("\n");


}

void freeQueue(queue** q){
  
    while((*q)->head!=NULL){
        node *delete = (*q)->head;
        (*q)->head = delete->next;
        free(delete->fileName);
        free(delete);
    }

    free(*q);
    
 

}

void freeSingleNode(node** q){
    free((*q)->fileName);
    free(*q);
}


