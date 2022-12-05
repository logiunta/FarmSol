#include <stdio.h>
#include <stdlib.h>
#include "results_utils.h"
#include "check_errors.h"
#include <string.h>

void initResQueue(resQueue** q){
    *q = malloc(sizeof(resQueue));
    CHECKNULL(*q,"malloc");
    (*q)->head = NULL;
    (*q)->size = 0;
}

void resEnqueueFront(resQueue** q,char *val, long sum){
    resFile *new = malloc(sizeof(resFile));
    CHECKNULL(new,"malloc");

    new->fileName = val;
    new->sum = sum;
    new->next = (*q)->head;
    
    (*q)->head = new;
    (*q)->size++;
}

resFile* resDequeueFront(resQueue** q){
  
    if((*q)->head == NULL){
        printf("lista vuota\n");
        return NULL;
    }
    
    resFile* tmp = (*q)->head;

    (*q)->head = ((*q)->head)->next;
    (*q)->size--;
   
    return tmp;
}

void resEnqueueBack(resQueue **q, char *val, long sum){
    resFile *tmp = malloc(sizeof(resFile));
    CHECKNULL(tmp,"malloc");
    resFile *currHead = (*q)->head;
    int len;
    if(val != 0){
            
        len = strlen(val);
        tmp->fileName = malloc(sizeof(char)*len+1);
        strncpy(tmp->fileName,val,len);
        tmp->fileName[len] = '\0';
        tmp->sum = sum;
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

int resQueueLen(resQueue *q){
    if(q == NULL)
        return 0;
    return q->size;
  
}


void freeResQueue(resQueue **q){
  
    while((*q)->head!=NULL){
        resFile *delete = (*q)->head;
        (*q)->head = delete->next;
        free(delete->fileName);
        free(delete);
    }

    free(*q);
    
 
}

void freeNode(resFile** q){
    free((*q)->fileName);
    free(*q);
}


void queueResultsDisplay(resQueue* q){
    printf("STAMPO LA LISTA:\n");
    resFile *curr = q->head;
    while(curr != NULL){
        printf("%ld ",curr->sum);
        printf("%s \n",curr->fileName);
    
        curr = curr->next;
    }
    printf("\n");

}