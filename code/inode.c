#include "../headers/inode.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

//*a few replacements to allign more with the new edited header file*/*

//replaced the old array because it mixed file data, directory and filnames in one struct
static inode_t inode_table[MAX_INODES];

void inode_table_init(void)
{
  memset(inode_table, 0, sizeof(inode_table));
}


//this is to prevent unsafe access to the array and reyturn NULL if outta bounds
inode_t *inode_get(uint32_t idx)
{
  if (idx >= MAX_INODES)
    return NULL;
  return &inode_table[idx];
}


//logic of allocating a new inode, and it uses the new "type" design from the header

int inode_alloc(uint8_t type)
{
  for (uint32_t i = 0; i < MAX_INODES; i++)
  {
    if (inode_table[i].type == INODE_FREE)
    {
      ///clears the previos data
      memset(&inode_table[i], 0, sizeof(inode_t));

      inode_table[i].type = type;

      ///more real filesystem-like timestamps
      time_t now = time(NULL);
      inode_table[i].last_access = now;
      inode_table[i].last_modified = now;
      inode_table[i].creation_time = now;


      if (type == INODE_DIR)
        //if this is a directory initialize entry count and set it to 0
        inode_table[i].dir.count = 0;
      return i;
        
      
      
    }
  }
  return -1;
}

///frees it
///
///maybe we should make it print an error but tbh im too lazy now maybe later
void inode_free(uint32_t idx)
{
  if (idx < MAX_INODES)
    memset(&inode_table[idx], 0, sizeof(inode_t));

}

///converts the inode to struct stat because this is what fuse expects (posix/unix file metadata)
void inode_to_stat(const inode_t *inode, struct stat *st, mode_t mode_extra)
{
  memset(st, 0, sizeof(struct stat));


  ////file types+permissions
  st->st_mode = mode_extra | 0755;
  if (mode_extra & S_IFDIR) {
    st->st_nlink = 2;
  } else {
    st->st_nlink = 1;
}
  st->st_size =  inode->size;
  st->st_atime = inode->last_access;
  st->st_mtime = inode->last_modified;
  st->st_ctime = inode->creation_time;


}


size_t inode_table_size(void)
{
    return sizeof(inode_table);
}
 
void *inode_table_ptr(void)
{
    return (void *)inode_table;
}





