#include "../headers/directory.h"
#include "../headers/inode.h"
#include <string.h>


/*we dont have a global array called filesystem, its called inode_table thats
  we will change all the lines mentioning it to that and inode_get
 */
////extern inode_t filesystem[max_files];
  ///commented out instead of deleting just to show whats wrong




// initialize a directory by setting its size to 0
void dir_init(uint32_t dir_idx) {
  inode_t *inode = inode_get(dir_idx);
  if (!inode) return;
  inode->size = 0;
}




// search for an entry by name in the directory
// returns the inode index if found, -1 if not found
int dir_lookup(uint32_t dir_idx, const char *name)
{
    inode_t *inode = inode_get(dir_idx);
    if (!inode) return -1;
    DirEntry *entries = (DirEntry *)inode->data;
    uint32_t count = dir_entry_count(dir_idx);
    for (uint32_t i = 0; i < count; i++) {
        if (strcmp(entries[i].name, name) == 0)
            return entries[i].inode_idx;
    }
    return -1;
}



// add a new directory entry with the given name and child inode index
// returns 0 on success, -1 if directory is full
int dir_add_entry(uint32_t dir_idx, const char *name, uint32_t child_idx)
{
  inode_t *inode = inode_get(dir_idx);
  if (!inode) return -1;
  if (inode->size + sizeof(DirEntry) > MAX_FILE_SIZE) 
    return -1;
  DirEntry *entries = (DirEntry *)inode->data;
  uint32_t count = dir_entry_count(dir_idx);      
  strncpy(entries[count].name, name, MAX_FILENAME);
  entries[count].name[MAX_FILENAME - 1] = '\0';
  entries[count].inode_idx = child_idx;
  inode->size += sizeof(DirEntry);                
   return 0;

}



// remove an entry from the directory by name
// sifts remaining entries up to fill the gap
// returns 0 on success, -1 if entry not found
int dir_remove_entry(uint32_t dir_idx, const char *name)
{
    inode_t *inode = inode_get(dir_idx);
    if (!inode) return -1;
    DirEntry *entries = (DirEntry *)inode->data;
    uint32_t count = dir_entry_count(dir_idx);  
    for (uint32_t i = 0; i < count; i++) {
        if (strcmp(entries[i].name, name) == 0) {
            for (uint32_t j = i; j < count - 1; j++)
                entries[j] = entries[j + 1];
            inode->size -= sizeof(DirEntry);  
            return 0;
        }
    }
    return -1;
}


// get a directory entry by index
// returns a pointer to the entry, or NULL if index is out of bounds
const DirEntry *dir_get_entry(uint32_t dir_idx, uint32_t index)
{
    inode_t *inode = inode_get(dir_idx);
    if (!inode || index >= dir_entry_count(dir_idx)) return NULL;
    DirEntry *entries = (DirEntry *)inode->data;
    return &entries[index];
}  



// get the number of entries in the directory
uint32_t dir_entry_count(uint32_t dir_idx)
{
    inode_t *inode = inode_get(dir_idx);
    if (!inode) return 0;
    return inode->size / sizeof(DirEntry);
}


// check if directory is empty
// returns 1 if empty, 0 if not empty

int dir_is_empty(uint32_t dir_idx) {
    inode_t *inode = inode_get(dir_idx);
    if (!inode) return 1;
    return inode->size == 0;
}
