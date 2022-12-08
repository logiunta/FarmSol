#include <stdio.h>
#include <stdlib.h>
#include "results_utils.h"
#include "check_errors.h"
#include <string.h>



void displayResults(resFile* arr,int size){
    for(int i=0;i<size;i++){
        printf("%ld ",(arr[i]).sum);
        printf("%s \n",(arr[i]).fileName);
    }
}

void addResult(resFile** arr,long val,char* fileName,int pos){


    if(*arr == NULL){
        *arr = (resFile*)malloc(sizeof(resFile));
        CHECKNULL(*arr,"malloc");
    }
                                
    else if(pos > 0){
        *arr = (resFile*)realloc(*arr,(pos+1)*sizeof(resFile));
        CHECKNULL(*arr,"realloc");
    }

    ((*arr)[pos]).sum = val;
   

    int len = strlen(fileName);
    ((*arr)[pos]).fileName = malloc(sizeof(char)*len+1);

    CHECKNULL(((*arr)[pos]).fileName,"malloc");

    strncpy(((*arr)[pos]).fileName,fileName,len);
    ((*arr)[pos]).fileName[len] = '\0';



}

 void freeResults(resFile** arr,int size){
    for(int i=0;i<size;i++){
        free(((*arr)[i]).fileName);

    }
    free(*arr);
}

static int compareSort (const void * a, const void * b) {
    resFile prec = *(const resFile*)a;
    resFile succ = *(const resFile*)b;

    return (prec.sum > succ.sum);
}

void sortResults(resFile** arr,int size){
    qsort(*arr,size,sizeof(resFile),compareSort);
    
}
