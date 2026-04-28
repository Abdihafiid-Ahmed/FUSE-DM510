#include "../headers/path.h"
#include "../headers/directory.h"
#include "../headers/inode.h"
#include <string.h>
#include <stdio.h>


///splits the path format x/y/z to an array like ["x", "y", "z"]
///
///

static int split_path(const char *path, char parts[][MAX_FILENAME], int max_parts)

{
  if (!path || path[0] != '/')
    return -1;

  int count = 0;
  ///skips the leading / cause path is always absolute
  const char *p = path + 1;

  while (*p && count < max_parts)
  {
    const char *slash = strchr(p,'/');
    size_t len;

    if (slash != NULL)
    {
      len = (size_t)(slash - p);

    }else{
      len = len = strlen(p);
    }
    
    ///just skipping empty components
    if (len == 0)
    {
      p++;
      continue;
    }

    if (len >= MAX_FILENAME)
    {
      fprintf(stderr, "path components is too long in '%s'\n", path);
      return -1;
    }

    strncpy(parts[count], p, len);
    parts[count][len] = '\0';
    count++;

    if (slash != NULL)
    {
      p = slash + 1;

    } else {
      break;

      
    }
  }
  return count;
}



///return inode index for full path or -1 if not found
///

int path_lookup(const char *path)
{
  //root always zero btw

  if (strcmp(path, "/") == 0)
    return ROOT_INODE_IDX;

  //temp storage for components

  char parts[32][MAX_FILENAME];
  int n = split_path(path, parts, 32);
  if (n < 0) 
    return -1;

  uint32_t current = ROOT_INODE_IDX;

  ///walk directory tree component by component 
  for (int i = 0; i < n; i++)
  {
    inode_t *inode = inode_get(current);
    if (!inode || inode->type != INODE_DIR)
    {
      return -1;
    }


    ///next entry
    int child = dir_lookup(current, parts[i]);
    if (child < 0)
    {
      return -1;

    }

    current = (uint32_t)child;
  }

  return (int)current;

}

///get parent inode and last name from path
///

int path_lookup_parent(const char *path, uint32_t *parent_idx, char *name_out)

{
  if (!path || path[0] != '/' || !parent_idx || !name_out)
      return -1;

  const char *last_slash = strrchr(path, '/');
  if (!last_slash) 
    return -1;

    //safer and used in unix file systems
  //also parent of /x is root
  char parent_path[4096];
  if (last_slash == path)
  {
    strcpy(parent_path, "/");

  } else {
    size_t parent_len = (size_t)(last_slash - path);
    if (parent_len >= sizeof(parent_path))
      return -1;

    strncpy(parent_path, path, parent_len);
    parent_path[parent_len] = '\0';

  }

  int p = path_lookup(parent_path);
  if (p < 0) 
    return -1;

  *parent_idx = (uint32_t)p;

  const char *name = last_slash + 1;
  if (strlen(name) == 0 || strlen(name) >= MAX_FILENAME)
    return -1;

  strncpy(name_out, name , MAX_FILENAME -1);
  name_out[MAX_FILENAME - 1] = '\0';
  return 0;
}


