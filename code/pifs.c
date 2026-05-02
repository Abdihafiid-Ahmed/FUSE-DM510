#define FUSE_USE_VERSION 26
 
#include <fuse.h>
#include <fuse/fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
 
#include "../headers/inode.h"
#include "../headers/directory.h"
#include "../headers/path.h"
#include "../headers/storage.h"

/* Forward declarations for file operations (implemented by lars by default, will be changed prolly) */
int pifs_open(const char *path, struct fuse_file_info *fi);
int pifs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int pifs_release(const char *path, struct fuse_file_info *fi);
void *pifs_init(void);   
void pifs_destroy(void *private_data);

/* Wrapper to adapt the no-arg pifs_init to the signature FUSE requires */
static void *pifs_init_wrapper(struct fuse_conn_info *conn) {
  (void)conn;
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

  return 0;
}

/*
 * Return one or more directory entries (struct dirent) to the caller. This is one of the most complex FUSE functions.
 * Required for essentially any filesystem, since it's what makes ls and a whole bunch of other things work.
 * The readdir function is somewhat like read, in that it starts at a given offset and returns results in a caller-supplied buffer.
 * However, the offset not a byte offset, and the results are a series of struct dirents rather than being uninterpreted bytes.
 * To make life easier, FUSE provides a "filler" function that will help you put things into the buffer.
 *
 * The general plan for a complete and correct readdir is:
 *
 * 1. Find the first directory entry following the given offset (see below).
 * 2. Optionally, create a struct stat that describes the file as for getattr (but FUSE only looks at st_ino and the file-type bits of st_mode).
 * 3. Call the filler function with arguments of buf, the null-terminated filename, the address of your struct stat
 *    (or NULL if you have none), and the offset of the next directory entry.
 * 4. If filler returns nonzero, or if there are no more files, return 0.
 * 5. Find the next file in the directory.
 * 6. Go back to step 2.
 * From FUSE's point of view, the offset is an uninterpreted off_t (i.e., an unsigned integer).
 * You provide an offset when you call filler, and it's possible that such an offset might come back to you as an argument later.
 * Typically, it's simply the byte offset (within your directory layout) of the directory entry, but it's really up to you.
 *
 * It's also important to note that readdir can return errors in a number of instances;
 * in particular it can return -EBADF if the file handle is invalid, or -ENOENT if you use the path argument and the path doesn't exist.
 */
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

  if (!dir_is_empty_count((uint32_t)idx))
    return -ENOTEMPTY;

  dir_remove_entry(parent_idx, name);
  inode_free((uint32_t)idx);
  return 0;
}

/*
 * See descriptions in fuse source code usually located in /usr/include/fuse/fuse.h
 * Notice: The version on Github may be a newer version than you have installed
 */
static struct fuse_operations pifs_oper = {
  .getattr  = pifs_getattr,
  .readdir  = pifs_readdir,
  .mknod    = NULL,
  .mkdir    = pifs_mkdir,
  .unlink   = NULL,
  .rmdir    = pifs_rmdir,
  .truncate = NULL,
  .open     = pifs_open,
  .read     = pifs_read,
  .release  = pifs_release,
  .write    = NULL,
  .rename   = NULL,
  .utime    = NULL,
  .init     = pifs_init_wrapper,
  .destroy  = pifs_destroy
};

/*
 * Open a file.
 * If you aren't using file handles, this function should just check for existence and permissions and return either success or an error code.
 * If you use file handles, you should also allocate any necessary structures and set fi->fh.
 * In addition, fi has some other fields that an advanced filesystem might find useful; see the structure definition in fuse_common.h for very brief commentary.
 * Link: https://github.com/libfuse/libfuse/blob/0c12204145d43ad4683136379a130385ef16d166/include/fuse_common.h#L50
 */
int pifs_open(const char *path, struct fuse_file_info *fi) {
  (void)fi;
  printf("open: (path=%s)\n", path);
  return 0;
}

/*
 * Read size bytes from the given file into the buffer buf, beginning offset bytes into the file. See read(2) for full details.
 * Returns the number of bytes transferred, or 0 if offset was at or beyond the end of the file. Required for any sensible filesystem.
 */
int pifs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  (void)size;
  (void)offset;
  (void)fi;
  printf("read: (path=%s)\n", path);
  memcpy(buf, "Hello\n", 6);
  return 6;
}

/*
 * This is the only FUSE function that doesn't have a directly corresponding system call, although close(2) is related.
 * Release is called when FUSE is completely done with a file; at that point, you can free up any temporarily allocated data structures.
 */
int pifs_release(const char *path, struct fuse_file_info *fi) {
  (void)fi;
  printf("release: (path=%s)\n", path);
  return 0;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the `private_data` field of
 * `struct fuse_context` to all file operations, and as a
 * parameter to the destroy() method. It overrides the initial
 * value provided to fuse_main() / fuse_new().
 */
void *pifs_init(void) {
  printf("init filesystem\n");
  return NULL;
}

/**
 * Clean up filesystem
 * Called on filesystem exit.
 */
void pifs_destroy(void *private_data) {
  (void)private_data;
  printf("destroy filesystem\n");
}

int main(int argc, char *argv[]) {
  fuse_main(argc, argv, &pifs_oper, NULL);
  return 0;
}
