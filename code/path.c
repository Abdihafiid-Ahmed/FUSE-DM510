#include "../headers/path.h"
#include "../headers/directory.h"
#include "../headers/inode.h"
#include <string.h>
#include <stdio.h>


///splits the path format x/y/z to an array like ["x", "y", "z"]
///
///

statin int split_path(const char *path, char parts[][MAX_FILENAME], int max_parts)

{
  if (!path || path[0] != '/')
    return -1;

  int count = 0;
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


