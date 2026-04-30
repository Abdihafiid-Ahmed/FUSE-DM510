#ifndef STORAGE_H
#define STORAGE_H


///file system signatire and version
#define PIFS_SIGNATURE "PiFS0001"

///length of signature no null terminator
#define PIFS_SIGNATURE_LEN  8


// Path to the SD card partition
#define DISK_PATH "/dev/mmcblk0p3"

// Load the file system from disk to memory
int storage_load(void);

// Save the file system from memory to disk
int storage_save(void);

#endif
