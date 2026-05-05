#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../headers/inode.h"
#include "../headers/directory.h"
#include "../headers/storage.h"

int main(void)
{
    //// choose target device (env override or default)
    const char *dev = getenv("PIFS_DEV");
    if (!dev) dev = DISK_PATH;

    //// print filesystem  info
    printf("pifs_format: formatting '%s'\n", dev);
    printf("  inode table size : %zu bytes\n", inode_table_size());
    printf("  max inodes       : %d\n", MAX_INODES);
    printf("  max file size    : %d bytes\n", MAX_FILE_SIZE);
    printf("  max dir entries  : %d\n", MAX_DIR_ENTRIES);

    ////empty filesystem
    inode_table_init();

    
    int root = inode_alloc(INODE_DIR);
    if (root != 0) {
        fprintf(stderr, "pifs_format: expected root at index 0, got %d\n", root);
        return EXIT_FAILURE;
    }
    dir_init((uint32_t)root);
    printf("  root directory created at inode %d\n", root);

    ////   open device/file and overwrite it completely
    FILE *fp = fopen(dev, "wb");
    if (!fp) {
        fprintf(stderr, "pifs_format: cannot open '%s': %s\n", dev, strerror(errno));
        return EXIT_FAILURE;
    }

    ////write signature to verify valid pifs partition
    if (fwrite(PIFS_SIGNATURE, 1, PIFS_SIGNATURE_LEN
, fp) != PIFS_SIGNATURE_LEN) {
        fprintf(stderr, "pifs_format: failed to write magic\n");
        fclose(fp);
        return EXIT_FAILURE;
    }

    
    size_t table_size = inode_table_size();
    size_t n = fwrite(inode_table_ptr(), 1, table_size, fp);
    fclose(fp);

    ////ensure full write succeeded
    if (n != table_size) {
        fprintf(stderr, "pifs_format: short write (%zu / %zu bytes)\n", n, table_size);
        return EXIT_FAILURE;
    }

    
    printf("pifs_format: wrote %zu bytes (magic + inode table)\n",
           PIFS_SIGNATURE_LEN
 + table_size);
    printf("pifs_format: done: partition is ready to mount\n");

    return EXIT_SUCCESS;
}
