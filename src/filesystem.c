#include "os.h"
#include "common.h"

/*------------------------------------------
                  kvfs.h
  ------------------------------------------*/

// static void kvfs_init(filesystem_t *this, const char *name) {
//   this->name = name;
//   inode_manager_init(&this->inode_manager);

// }

// static int kvfs_access(filesystem_t *this, const char *path, int mode) {

// }

// static int kvfs_open(filesystem_t *this, const char *path, int flags) {
//   int create = (flags & O_CREAT) ? 1 : 0;
//   inode_t *inode = inode_manager_lookup(&this->inode_manager, path, 
//     INODE_FILE, create, DEFAULT_MODE);
  
// }

// filesystem_t *new_kvfs(const char *name) {
//   filesystem_t *kvfs = pmm->alloc(sizeof(filesystem_t));
//   kvfs_init(kvfs, name);
//   kvfs->access = kvfs_access;
//   kvfs->open = kvfs_open;
// }