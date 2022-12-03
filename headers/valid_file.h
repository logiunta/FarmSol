#ifndef VALID_FILE
#define VALID_FILE


int is_regular_file(const char *path,struct stat *path_stat);
char* fileExtension(const char *path);
long getFileSize(const char* path,struct stat *path_stat);
int is_executable_file(const char* path,struct stat *path_stat);
int isBinaryFile(const char* path,struct stat *path_stat);
int is_a_valid_file(const char* path,struct stat *path_stat);

#endif