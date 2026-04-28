#ifndef PATH_H
#define PATH_H

#include "inode.h"

//added this to make path.c lookup more readable
//
//
#define ROOT_INODE_IDX 0



// Find the inode index for a full path
//
// i have changed get_inode to path_lookup because we already have a get_inode
// functoin declared in inode.h which will cause issues in code readability

int path_lookup(const char *path);

// Find the parent folder's inode and get the file name
// i changed *filename to name_out because we are also deailng with directories too and these arent files
int path_lookup_parent(const char *path, uint32_t *parent_idx, char *name_out);

#endif
