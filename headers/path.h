#ifndef PATH_H
#define PATH_H

#include "inode.h"

// Find the inode index for a full path
int get_inode(const char *path);

// Find the parent folder's inode and get the file name
int get_parent(const char *path, char *filename);

#endif