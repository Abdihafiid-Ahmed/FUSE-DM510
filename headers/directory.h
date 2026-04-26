#ifndef DIRECTORY_H
#define DIRECTORY_H
#include "inode.h"
#include <stdint.h>

/*in the dir.h and .c we will deal with inode indexes
 *instead of paths because in the design we
 *will have a path file that will handle pathes (path.c) 
 * */ 


//initialize
void dir_init(uint32_t dir_idx);


//return inode index of "name""
int dir_lookup(uint32_t dir_idx, const char *name);


///add name into child index
int dir_add_entry(uint32_t, const char *name, uint32_t child_idx);


///remove name "self explanatory"
int dir_remove_entry(uint32_t dir_idx, const char *name);

///return the entry of an index
const DirEntry *dir_get_entry(uint32_t dir_idx, uint32_t index);


///return number of of entries in directory
uint32_t dir_entry_count(uint32_t dir_idx);


//returm 1 if empty and zero if not
int dir_is_empty_count(uint32_t dir_idx);


#endif 
