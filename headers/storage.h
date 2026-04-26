#ifndef STORAGE_H
#define STORAGE_H

// Path to the SD card partition
#define DISK_PATH "/dev/mmcblk0p3"

// Load the file system from disk to memory
int storage_load(void);

// Save the file system from memory to disk
int storage_save(void);

#endif