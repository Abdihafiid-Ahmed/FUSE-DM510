#include "directory.h"
#include <string.h>

extern inode_t filesystem[MAX_FILES];

// initialize a directory by setting its size to 0
void dir_init(uint32_t dir_idx) {
    filesystem[dir_idx].size = 0;
}

// search for an entry by name in the directory
// returns the inode index if found, -1 if not found
int dir_lookup(uint32_t dir_idx, const char *name) {
    DirEntry *entries = (DirEntry *)filesystem[dir_idx].data;
    uint32_t count = dir_entry_count(dir_idx);

    for (uint32_t i = 0; i < count; i++) {
        if (strcmp(entries[i].name, name) == 0) {
            return entries[i].inode_idx;
        }
    }
    return -1;
}

// add a new directory entry with the given name and child inode index
// returns 0 on success, -1 if directory is full
int dir_add_entry(uint32_t dir_idx, const char *name, uint32_t child_idx) {
    if (filesystem[dir_idx].size + sizeof(DirEntry) > MAX_FILE_SIZE) {
        return -1; 
    }

    DirEntry *entries = (DirEntry *)filesystem[dir_idx].data;
    uint32_t count = dir_entry_count(dir_idx);

    strncpy(entries[count].name, name, MAX_FILENAME);
    entries[count].name[MAX_FILENAME - 1] = '\0';  // Ensure null-termination
    entries[count].inode_idx = child_idx;
    filesystem[dir_idx].size += sizeof(DirEntry);
    
    return 0;
}

// remove an entry from the directory by name
// sifts remaining entries up to fill the gap
// returns 0 on success, -1 if entry not found
int dir_remove_entry(uint32_t dir_idx, const char *name) {
    DirEntry *entries = (DirEntry *)filesystem[dir_idx].data;
    uint32_t count = dir_entry_count(dir_idx);

    for (uint32_t i = 0; i < count; i++) {
        if (strcmp(entries[i].name, name) == 0) {
            // Shift remaining entries up to fill the gap
            for (uint32_t j = i; j < count - 1; j++) {
                entries[j] = entries[j + 1];
            }
            filesystem[dir_idx].size -= sizeof(DirEntry);
            return 0;
        }
    }
    return -1;
}

// get a directory entry by index
// returns a pointer to the entry, or NULL if index is out of bounds
const DirEntry *dir_get_entry(uint32_t dir_idx, uint32_t index) {
    if (index >= dir_entry_count(dir_idx)) {
        return NULL; 
    }

    DirEntry *entries = (DirEntry *)filesystem[dir_idx].data;
    return &entries[index];
}

// get the number of entries in the directory
uint32_t dir_entry_count(uint32_t dir_idx) {
    return filesystem[dir_idx].size / sizeof(DirEntry);
}

// check if directory is empty
// returns 1 if empty, 0 if not empty
int dir_is_empty_count(uint32_t dir_idx) {
    return (filesystem[dir_idx].size == 0) ? 1 : 0;
}