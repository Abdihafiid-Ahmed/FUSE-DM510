#ifndef INODE_H
#define INODE_H

#include <time.h>

#define MAX_FILES 100
#define MAX_FILENAME 32
#define MAX_FILE_SIZE 4096

typedef struct {
    int in_use;
    int is_dir;
    char name[MAX_FILENAME];
    int size;
    char data[MAX_FILE_SIZE];
    time_t last_modified;
} inode_t;

#endif

