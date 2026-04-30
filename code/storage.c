#include "../headers/storage.h"
#include "../headers/inode.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

 

///get device path from env or if not fall back on default

static const char *dev_path(void)
{
  const char *env = getenv("PIFS_DEV");

  if (env != NULL)
  {
    return env;

  } else {
      return DISK_PATH;

  }
}


int storage_load(void)
{
  const char *path = dev_path();
  FILE *fp = fopen(path, "rb");

  if (!fp)
  {
    fprintf(stderr, "storage_load: cannot open '%s': %s\n", path, strerror(errno));

    return -1;

  }


  ///read signature from dish

  char sig[PIFS_SIGNATURE_LEN];

  if (fread(sig, 1, PIFS_SIGNATURE_LEN, fp) != PIFS_SIGNATURE_LEN)
  {
    fprintf(stderr, "storage_load: failed to read signature\n");
    fclose(fp);
    return -1;

  }


  ///verift the signature

  if (memcmp(sig, PIFS_SIGNATURE, PIFS_SIGNATURE_LEN) != 0)
  {
    fprintf(stderr, "storage_load: invalid filesystem\n");
    fclose(fp);
    return -1;
  }

  ///load inode table 
  
  size_t table_size = inode_table_size();
  size_t n = fread(inode_table_ptr(), 1, table_size, fp);
  fclose(fp);

  if (n != table_size)
  {
    fprintf(stderr, "storage_load: expected %zu bytes, got %zu\n", table_size, n);
    return -1;
    
  }

  return 0;
}

int storage_save(void)
{
  const char *path = dev_path();
  FILE *fp = fopen(path, "wb");

  if (!fp)
  {
    fprintf(stderr, "storage_save: cant open '%s': %s\n", path, strerror(errno));
    return -1;
  }

  ///write fs signaure 

  if (fwrite(PIFS_SIGNATURE, 1, PIFS_SIGNATURE_LEN, fp) != PIFS_SIGNATURE_LEN)
  {
    fprintf(stderr, "storage_save; failed to write signaure\n");
    fclose(fp);
    return -1;

  }

  ///write inode table

  size_t table_size = inode_table_size();
  size_t n = fwrite(inode_table_ptr(), 1, table_size, fp);
  fclose(fp);

  if (n != table_size)
  {
    fprintf(stderr, "storage_save: incomplete write (%zu /%zu bytes)\n", n, table_size);
    return -1;
  }

  return 0;

}
