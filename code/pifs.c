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

/* Forward declarations for file operations (implemented by lars by default, will be changed prolly) 
int pifs_open(const char *path, struct fuse_file_info *fi);
int pifs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int pifs_release(const char *path, struct fuse_file_info *fi);
void *pifs_init(void);   
void pifs_destroy(void *private_data);

*/



////Initialize

void *pifs_init(void)
{
  fprintf(stderr, "init filesystem\n");
//(small fix) without it getattr fails   
  if (storage_load() != 0) {
        fprintf(stderr, "No existing filesystem found or load failed. creating new one...\n");
        inode_table_init();
        int root = inode_alloc(INODE_DIR);
        dir_init((uint32_t)root);
    } else {
        fprintf(stderr, "Filesystem loaded successfully from storage.\n");
    }

    return NULL;
}

///Wrapper to adapt the pifs_init to the signature FUSE requires 
static void *pifs_init_wrapper(struct fuse_conn_info *conn) {
  (void)conn;

  storage_save();
  return pifs_init();
}


/*
 * Return file attributes.
 * The "stat" structure is described in detail in the stat(2) manual page.
 * For the given pathname, this should fill in the elements of the "stat" structure.
 * If a field is meaningless or semi-meaningless (e.g., st_ino) then it should be set to 0 or given a "reasonable" value.
 * This call is pretty much required for a usable filesystem.
 */
static int pifs_getattr(const char *path, struct stat *stbuf)
{
  //logs the path
  fprintf(stderr, "getattr %s\n", path);

  ///resolves it to inode index
  int idx = path_lookup(path);
  if (idx < 0)
    return -ENOENT;

  ///extracts the inode from storage
  inode_t *inode = inode_get((uint32_t)idx);
  if (!inode)
    return -ENOENT;

  if (inode->type == INODE_DIR)
    inode_to_stat(inode, stbuf, S_IFDIR);
  else
    inode_to_stat(inode, stbuf, S_IFREG);

  storage_save();
  return 0;
}


static int pifs_readdir(const char *path,
                        void *buf,
                        fuse_fill_dir_t filler,
                        off_t offset,
                        struct fuse_file_info *fi)
{
  (void)offset;
  (void)fi;
  ///lists content of directory and always adds "." and ".." at first then iterates over the direntry array
  fprintf(stderr, "readdir: %s\n", path);

  int idx = path_lookup(path);
  if (idx < 0)
    return -ENOENT;

  inode_t *inode = inode_get((uint32_t)idx);
  if (!inode || inode->type != INODE_DIR)
    return -ENOTDIR;

  ///filler is a fuse callback that adds entries to the directory buffer
  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  uint32_t count = dir_entry_count((uint32_t)idx);
  for (uint32_t i = 0; i < count; i++)
  {
    const DirEntry *entry = dir_get_entry((uint32_t)idx, i);
    if (!entry) break;
    filler(buf, entry->name, NULL, 0);
  }
  
  storage_save();
  return 0;
}

///create a new sub directory
static int pifs_mkdir(const char *path, mode_t mode)
{
  (void)mode;
  fprintf(stderr, "mkdir: %s\n", path);

  uint32_t parent_idx;
  char name[MAX_FILENAME];
  if (path_lookup_parent(path, &parent_idx, name) < 0)
    return -ENOENT;

  //prevent duplicate directory names within the same parent
  if (dir_lookup(parent_idx, name) >= 0)
    return -EEXIST;

  int new_idx = inode_alloc(INODE_DIR);
  if (new_idx < 0)
    return -ENOSPC;

  dir_init((uint32_t)new_idx);

  if (dir_add_entry(parent_idx, name, (uint32_t)new_idx) < 0) {
    inode_free((uint32_t)new_idx);
    return -ENOSPC;
  }
  
  storage_save();
  return 0;
}

////removes an empty directory
static int pifs_rmdir(const char *path)
{
  fprintf(stderr, "rmdir: %s\n", path);

  uint32_t parent_idx;
  char name[MAX_FILENAME];
  if (path_lookup_parent(path, &parent_idx, name) < 0)
    return -ENOENT;

  int idx = dir_lookup(parent_idx, name);
  if (idx < 0)
    return -ENOENT;

  inode_t *inode = inode_get((uint32_t)idx);
  if (!inode || inode->type != INODE_DIR)
    return -ENOTDIR;

  if (!dir_is_empty((uint32_t)idx))
    return -ENOTEMPTY;

  dir_remove_entry(parent_idx, name);
  inode_free((uint32_t)idx);

  storage_save();
  return 0;
}

///files
static int pifs_mknod(const char *path, mode_t mode, dev_t rdev) {
    (void)mode;
    (void)rdev;
    fprintf(stderr, "mknod: %s\n", path);

    uint32_t parent_idx;
    char name[MAX_FILENAME];

    if (path_lookup_parent(path, &parent_idx, name) < 0) return -ENOENT;

  ///prevents duplicates
    if (dir_lookup(parent_idx, name) >= 0) return -EEXIST;

    int new_idx = inode_alloc(INODE_FILE); 
    if (new_idx < 0) return -ENOSPC;

    inode_t *new_inode = inode_get((uint32_t)new_idx);
    new_inode->size = 0;

    if (dir_add_entry(parent_idx, name, (uint32_t)new_idx) < 0) {
        inode_free((uint32_t)new_idx);
        return -ENOSPC;
    }
    
    storage_save();
    return 0;
}

static int pifs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    return pifs_mknod(path, mode, 0);
}

///delete files
static int pifs_unlink(const char *path) 

{
    fprintf(stderr, "unlink: %s\n", path);

    uint32_t parent_idx;
    char name[MAX_FILENAME];

    if (path_lookup_parent(path, &parent_idx, name) < 0) return -ENOENT;

    int idx = dir_lookup(parent_idx, name);
    if (idx < 0) return -ENOENT;

    inode_t *inode = inode_get((uint32_t)idx);

  //prevents deleting directories
    if (!inode || inode->type == INODE_DIR) return -EISDIR;

    dir_remove_entry(parent_idx, name);
    inode_free((uint32_t)idx);
    
    storage_save();
    return 0;
}


///open files
static int pifs_open(const char *path, struct fuse_file_info *fi) {
    (void)fi;
    fprintf(stderr, "open: %s\n", path);

    int idx = path_lookup(path);
    if (idx < 0) return -ENOENT;

    inode_t *inode = inode_get((uint32_t)idx);
    if (!inode || inode->type == INODE_DIR) return -EISDIR;
    ///upadate accessn time 
    inode->last_access = time(NULL);

    storage_save();

    return 0;
}

///read files
static int pifs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void)fi;
    fprintf(stderr, "read: %s\n", path);

    int idx = path_lookup(path);
    if (idx < 0) return -ENOENT;

    inode_t *inode = inode_get((uint32_t)idx);
    if (!inode || inode->type == INODE_DIR) return -EISDIR;

    if (offset >= (off_t)inode->size) return 0; 

  ///prevents reading beyond file size
    if ((size_t)offset + size > inode->size)
    size = inode->size - (size_t)offset;

    memcpy(buf, inode->data + offset, size);
    //update time
    inode->last_access = time(NULL);

    storage_save();
    return (int)size;
}


static int pifs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void)fi;
    fprintf(stderr, "write: %s\n", path);

    int idx = path_lookup(path);
    if (idx < 0) return -ENOENT;

    inode_t *inode = inode_get((uint32_t)idx);
    if (!inode || inode->type == INODE_DIR) return -EISDIR;

    if ((size_t)offset + size > MAX_FILE_SIZE)
        size = MAX_FILE_SIZE - (size_t)offset;
    if (size == 0) return -ENOSPC;
    memcpy(inode->data + offset, buf, size);

    // extend file size if write goes past current end
    if ((size_t)offset + size > inode->size)
        inode->size = (size_t)offset + size;

    // update modification time after write
    inode->last_modified = time(NULL);

    storage_save();
    return (int)size;
}


// resize file
static int pifs_truncate(const char *path, off_t size) {
    fprintf(stderr, "truncate: %s\n", path);
    int idx = path_lookup(path);
    if (idx < 0) return -ENOENT;
    inode_t *inode = inode_get((uint32_t)idx);


    if (!inode || inode->type == INODE_DIR) return -EISDIR;
    if (size < 0 || (size_t)size > MAX_FILE_SIZE) return -EINVAL;


    // zero out freed region so reads don't see stale data
    if ((size_t)size < inode->size)
        memset(inode->data + size, 0, inode->size - (size_t)size);
    inode->size = (size_t)size;


    // update modification time on truncate
    inode->last_modified = time(NULL);
    
    storage_save();
    return 0;
}


int pifs_release(const char *path, struct fuse_file_info *fi) {
  (void)fi;
  printf("release: (path=%s)\n", path);

  storage_save();
  return 0;
}


void pifs_destroy(void *private_data) {
  (void)private_data;
  fprintf(stderr, "destroy filesystem\n");
  if (storage_save() == 0) {
        fprintf(stderr, "Save successful.\n");
    } else {
        fprintf(stderr, "Save FAILED!\n");
    }
}
static struct fuse_operations pifs_oper = {
  .getattr  = pifs_getattr,
  .readdir  = pifs_readdir,
  .create   = pifs_create,
  .mkdir    = pifs_mkdir,
  .unlink   = pifs_unlink,
  .rmdir    = pifs_rmdir,
  .truncate = pifs_truncate,
  .open     = pifs_open,
  .read     = pifs_read,
  .release  = pifs_release,
  .write    = pifs_write,
 
  .init     = pifs_init_wrapper,
  .destroy  = pifs_destroy
};


int main(int argc, char *argv[]) {
  fuse_main(argc, argv, &pifs_oper, NULL);
  return 0;
}
