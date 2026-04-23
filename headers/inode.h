#ifndef INODE_H
#define INODE_H

#include <stdint.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stddef.h>

///changed from files to INODES and from 100 to 128 because its a power of two and much prefered in systems code also 128*32kb = 4mb
#define MAX_INODES 128 
#define MAX_FILENAME 32
//i changed this from 4096 (4kb) to 32*1024 (32kb) big enough to hold the metadatas 
#define MAX_FILE_SIZE (32*1024)
//added a max directories two
#define MAX_DIR_ENTRIES 32



///define 3 types of inodes if its free, or a file or a directoy
#define INODE_FREE  0
#define INODE_FILE  1
#define INODE_DIR   2


///added a directory entry too because only files isnt enough

typedef struct
{
  char  name[MAX_FILENAME];
  uint32_t inode_idx;
} DirEntry;




/*changed in_use and is_dir and replaced it with type, 
which will  be easeir to debug,
i also replaced all int with unsigned int because its more logical
since it doesnt accept negative numbers*/

typedef struct {
    uint8_t type;   ///Inode types 
    uint32_t size;

/*added creation time and last asccessed too 
on top of the last modified so it can resemble a real file more */

    time_t last_access;
    time_t last_modified;
    time_t creation_time;

/*removed the file name too because they arent supposed to 
 * be stored in the inode*/

    //union helps us use the same memory for 2 different things ie file or dir
    //check geeksforgeeks website for more elaboration if you need

    union {
      uint8_t data[MAX_FILE_SIZE];

      struct{
        DirEntry entries[MAX_DIR_ENTRIES];
        uint32_t count;
      } dir;
    };
     
} inode_t;








//initializing
void inode_table_init(void);

//returns a pointer to inode at index or null if its out of range
inode_t *inode_get(uint32_t idx);

//allocates a free inode
int inode_alloc(uint8_t type);

//releases inode
void inode_free(uint32_t idx);

//converts our inode to the stat template, much needed in getattr later on
void inode_to_stat(const inode_t *inode, struct stat *st, mode_t mode_extra);

///returns total memory size of inode table
size_t inode_table_size(void);

////returns raw pointer to inode table memory very much needed later on tpp
void *inode_table_ptr(void);

#endif
  
