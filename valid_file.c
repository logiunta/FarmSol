#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>


int is_regular_file(const char *path, struct stat *path_stat){
    return S_ISREG((*path_stat).st_mode);
} // char* ext = fileExtension(path);
    // if((strncmp(ext,"dat",strlen(ext)) != 0) && (strncmp(ext,"",strlen(ext)) != 0))
    //     return 0;


char* fileExtension(const char *path){
    char *s = strrchr(path, '.');
    int index = (int)(s - path);
    //il punto in testa alla stringa indica la directory corrente, lo devo escludere
    if(index == 0)
        return "";

    if(!s ) return "";
    return s + 1;
}

long getFileSize(const char* path,struct stat *path_stat){
    long size = 0;
   
    size = (*path_stat).st_size;
    return size;
}
int is_executable_file(const char* path,struct stat *path_stat){
    
    if ((*path_stat).st_mode & S_IXUSR) 
        return 1;

    return 0;
}

int isBinaryFile(const char* path,struct stat *path_stat){
    // char* ext = fileExtension(path);
    // if((strncmp(ext,"dat",strlen(ext)) != 0) && (strncmp(ext,"",strlen(ext)) != 0))
    //     return 0;
    long filelen = getFileSize(path,path_stat);

    if((filelen % 8) != 0)
        return 0;

    return 1;

}

int is_a_valid_file(const char* path,struct stat *path_stat){
    int res = stat(path, path_stat);

    if(res == -1)
        return res;
    

    return (isBinaryFile(path,path_stat) && is_regular_file(path,path_stat) && !is_executable_file(path,path_stat));
        

}