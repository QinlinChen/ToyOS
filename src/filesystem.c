#include "os.h"
#include "common.h"

/*------------------------------------------
                  kvfs.h
  ------------------------------------------*/

void kvfs_init(filesystem_t *kvfs, const char *name) {
  strcpy(kvfs->name, name);
  inode_manager_init(&kvfs->inode_manager);

}

