#ifndef RESULTS_UTILS
#define RESULTS_UTILS

typedef struct ResFile{
    long sum;
    char* fileName;

}resFile;


void addResult(resFile** arr,long val,char* fileName, int pos);
void displayResults(resFile* arr,int size);
void freeResults(resFile** arr,int size);
void sortResults(resFile** arr,int size);



#endif

