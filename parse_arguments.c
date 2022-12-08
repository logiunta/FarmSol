#define _POSIX_C_SOURCE 200112L
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "queue_utils.h"
#include "check_errors.h"
#include <errno.h>
#include <dirent.h>
#include "valid_file.h"
#include "parse_arguments.h"

#define MAXFILENAME 255


static int is_dir(const char *path,struct stat *path_stat){
    int res = stat(path,path_stat);
    if(res == -1)
        return res;
        
    return S_ISDIR((*path_stat).st_mode);
}


void goInDir(char*path,queue** list,int *errors,struct stat *path_stat){
    DIR* dir;
    int err = -1;
    int res;
    int len = 0;

    dir = opendir(path);
    CHECKNULL(dir,"openDir");
    struct dirent* file;   

    while ((file = readdir(dir))!=NULL) {
        len = strlen(file->d_name) + strlen(path)+1;
        char relativePath[len+1];
        strncpy(relativePath,path,len);
        strncat(relativePath,"/",len);
        strncat(relativePath,file->d_name,len);
        len = strlen(relativePath);
        relativePath[len] = '\0';

        if (strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0){
            res = is_dir(relativePath,path_stat);
            if(res == -1){
                SYSCALL(err,closedir(dir),"closeDir");
                perror("is_dir");
                exit(EXIT_FAILURE);
            }
                
            else if(res == 1){
                goInDir(relativePath,list,errors,path_stat);
            }

            else {
                
                res = is_a_valid_file(relativePath,path_stat);

                if(res == -1){
                    SYSCALL(err,closedir(dir),"closeDir");
                    perror("is_a_valid_file");
                    exit(EXIT_FAILURE);

                }

                if(res)
                    enqueueBack(list,relativePath,-1);
                
                else 
                    (*errors)++;
                

            }
        }
    }

    
    SYSCALL(err,closedir(dir),"closeDir");
 
}


//check arguments and options
int parseArguments(int argc, char* argv[],int *n,int *q, long *t,queue** list){
    int opt,len;
    int errors = 0;
    int res;
    char file[MAXFILENAME],*p;
    struct stat path_stat;


    if(argc < 2){
        fprintf(stderr,"Too few arguments:\nUsage %s [OPTION]... [FILE]... \n",argv[0]);
        return -1;
    }
  

    while (optind <= argc) {
    
        if((opt = getopt(argc, argv, ":n:q:d:t:")) != -1) {
        
            switch (opt)
            {
                case 'n':
                case 'q':
                    int num = atoi(optarg);
                    if(num == 0){
                        fprintf(stderr,"Error: %s is not a valid argument for option -%c\n",optarg,opt);
                        return -1;
                    }
                    
                    if(opt == 'n')
                        *n = num;

                    else *q = num;
                                
                    break;

                case 't':
                    long delay = strtol(optarg,&p,10);
                    if(delay == 0){
                        fprintf(stderr,"Error: %s is not a valid argument for option -%c\n",optarg,opt);
                        return -1;
                    }

                    *t = delay;
                    break; 

                case 'd':

                    if(is_dir(optarg,&path_stat) != 1){
                        fprintf(stderr,"Error: %s is not a dir \n",optarg);
                        return -1;

                    }

                    goInDir(optarg,list,&errors,&path_stat);
                    
                    break;

                case ':': 
                    fprintf(stderr,"Option -%c needs an argument\n",optopt); 
                    return -1;
                    

                case '?': 
                    if(optopt == '-'){
                        fprintf(stderr,"Unknown option: %c\n", optopt);
                        return -1;
                    }
                    break;
            }
        }

        else{
            break;
        }
        
    }   

 
    //not option arguments
    
    for (; optind < argc; optind++){
        len = strlen(argv[optind]) +1 ;
        strncpy(file,argv[optind],len);
        file[len] = '\0';
        res = is_a_valid_file(file,&path_stat);

        //error on stat
        if(res == -1)
            fprintf(stderr,"File %s not exist\n",file);
            
        else if(res){
            enqueueBack(list,file,-1);
        }
        
        else errors++;
      
    }  

  
    //found some files with incorrect format 
    if(errors != 0)
        fprintf(stderr,"Found some files with incorrect format: read only regular and binary files\n");

    return 0;
}

  


