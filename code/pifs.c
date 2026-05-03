#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../headers/inode.h"
#include "../headers/directory.h"
#include "../headers/path.h"
#include "../headers/storage.h"

/* --- Initialization & Teardown --- */

void *pifs_init(void) {
    fprintf(stderr, "init filesystem\n");
    // TODO later: Call storage_load() here!
    return NULL;
}

void pifs_destroy(void *private_data) {
    (void)private_data;
    fprintf(stderr, "destroy filesystem\n");
}

static void *pifs_init_wrapper(struct fuse_conn_info *conn) {
    (void)conn;
    return pifs_init();
}

/* --- Core FUSE Operations --- */

static int pifs_getattr(const char *path, struct stat *stbuf) {
    fprintf(stderr, "getattr %s\n", path);

    int idx = path_lookup(path);
    if (idx < 0) return -ENOENT;

    inode_t *inode = inode_get((uint32_t)idx);
    if (!inode) return -ENOENT;

    if (inode->type == INODE_DIR)
        inode_to_stat(inode, stbuf, S_IFDIR);
    else
        inode_to_stat(inode, stbuf, S_IFREG);

    return 0;
}

static int pifs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void)offset;
    (void)fi;
    fprintf(stderr, "readdir: %s\n", path);

    int idx = path_lookup(path);
    if (idx < 0) return -ENOENT;

    inode_t *inode = inode_get((uint32_t)idx);
    if (!inode || inode->type != INODE_DIR) return -ENOTDIR;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    uint32_t count = dir_entry_count((uint32_t)idx);
    for (uint32_t i = 0; i < count; i++) {
        const DirEntry *entry = dir_get_entry((uint32_t)idx, i);
        if (!entry) break;
        filler(buf, entry->name, NULL, 0);
    }

    return 0;
}

static int pifs_mkdir(const char *path, mode_t mode) {
    (void)mode;
    fprintf(stderr, "mkdir: %s\n", path);

    uint32_t parent_idx;
    char name[MAX_FILENAME];
    if (path_lookup_parent(path, &parent_idx, name) < 0) return -ENOENT;

    if (dir_lookup(parent_idx, name) >= 0) return -EEXIST;

    int new_idx = inode_alloc(INODE_DIR);
    if (new_idx < 0) return -ENOSPC;

    dir_init((uint32_t)new_idx);

    if (dir_add_entry(parent_idx, name, (uint32_t)new_idx) < 0) {
        inode_free((uint32_t)new_idx);
        return -ENOSPC;
    }
    
    // TODO later: Call storage_save() here!
    return 0;
}

static int pifs_rmdir(const char *path) {
    fprintf(stderr, "rmdir: %s\n", path);

    uint32_t parent_idx;
    char name[MAX_FILENAME];
    if (path_lookup_parent(path, &parent_idx, name) < 0) return -ENOENT;

    int idx = dir_lookup(parent_idx, name);
    if (idx < 0) return -ENOENT;

    inode_t *inode = inode_get((uint32_t)idx);
    if (!inode || inode->type != INODE_DIR) return -ENOTDIR;

    if (!dir_is_empty_count((uint32_t)idx)) return -ENOTEMPTY;

    dir_remove_entry(parent_idx, name);
    inode_free((uint32_t)idx);
    
    // TODO later: Call storage_save() here!
    return 0;
}

static int pifs_mknod(const char *path, mode_t mode, dev_t rdev) {
    (void)mode;
    (void)rdev;
    fprintf(stderr, "mknod: %s\n", path);

    uint32_t parent_idx;
    char name[MAX_FILENAME];

    if (path_lookup_parent(path, &parent_idx, name) < 0) return -ENOENT;
    if (dir_lookup(parent_idx, name) >= 0) return -EEXIST;

    int new_idx = inode_alloc(INODE_FILE); 
    if (new_idx < 0) return -ENOSPC;

    inode_t *new_inode = inode_get((uint32_t)new_idx);
    new_inode->size = 0;

    if (dir_add_entry(parent_idx, name, (uint32_t)new_idx) < 0) {
        inode_free((uint32_t)new_idx);
        return -ENOSPC;
    }
    
    // TODO later: Call storage_save() here!
    return 0;
}

static int pifs_unlink(const char *path) {
    fprintf(stderr, "unlink: %s\n", path);

    uint32_t parent_idx;
    char name[MAX_FILENAME];

    if (path_lookup_parent(path, &parent_idx, name) < 0) return -ENOENT;

    int idx = dir_lookup(parent_idx, name);
    if (idx < 0) return -ENOENT;

    inode_t *inode = inode_get((uint32_t)idx);
    if (!inode || inode->type == INODE_DIR) return -EISDIR;

    dir_remove_entry(parent_idx, name);
    inode_free((uint32_t)idx);
    
    // TODO later: Call storage_save() here!
    return 0;
}

static int pifs_open(const char *path, struct fuse_file_info *fi) {
    (void)fi;
    fprintf(stderr, "open: %s\n", path);

    int idx = path_lookup(path);
    if (idx < 0) return -ENOENT;

    inode_t *inode = inode_get((uint32_t)idx);
    if (!inode || inode->type == INODE_DIR) return -EISDIR;

    return 0;
}

static int pifs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void)fi;
    fprintf(stderr, "read: %s\n", path);

    int idx = path_lookup(path);
    if (idx < 0) return -ENOENT;

    inode_t *inode = inode_get((uint32_t)idx);
    if (!inode || inode->type == INODE_DIR) return -EISDIR;

    if (offset >= inode->size) return 0; 
    if (offset + size > inode->size) size = inode->size - offset;

    memcpy(buf, inode->data + offset, size);
    return size;
}

static int pifs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void)fi;
    fprintf(stderr, "write: %s\n", path);

    int idx = path_lookup(path);
    if (idx < 0) return -ENOENT;

    inode_t *inode = inode_get((uint32_t)idx);
    if (!inode || inode->type == INODE_DIR) return -EISDIR;

    if (offset + size > MAX_FILE_SIZE) size = MAX_FILE_SIZE - offset;
    if (size == 0) return -ENOSPC; 

    memcpy(inode->data + offset, buf, size);

    if (offset + size > inode->size) inode->size = offset + size;
    
    // TODO later: Call storage_save() here!
    return size; 
}

static int pifs_truncate(const char *path, off_t size) {
    fprintf(stderr, "truncate: %s\n", path);

    int idx = path_lookup(path);
    if (idx < 0) return -ENOENT;

    inode_t *inode = inode_get((uint32_t)idx);
    if (!inode || inode->type == INODE_DIR) return -EISDIR;

    if (size > MAX_FILE_SIZE) return -EINVAL; 

    inode->size = size;
    
    // TODO later: Call storage_save() here!
    return 0;
}

static int pifs_release(const char *path, struct fuse_file_info *fi) {
    (void)fi;
    fprintf(stderr, "release: %s\n", path);
    return 0;
}

/* --- FUSE Operations Struct --- */

static struct fuse_operations pifs_oper = {
    .getattr  = pifs_getattr,
    .readdir  = pifs_readdir,
    .mknod    = pifs_mknod,     // Linked!
    .mkdir    = pifs_mkdir,
    .unlink   = pifs_unlink,    // Linked!
    .rmdir    = pifs_rmdir,
    .truncate = pifs_truncate,  // Linked!
    .open     = pifs_open,
    .read     = pifs_read,
    .release  = pifs_release,   // Linked!
    .write    = pifs_write,     // Linked!
    .rename   = NULL,
    .utime    = NULL,
    .init     = pifs_init_wrapper,
    .destroy  = pifs_destroy
};

/* --- Main Program --- */

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &pifs_oper, NULL);
}