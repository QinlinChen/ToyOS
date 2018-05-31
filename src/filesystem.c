#include "os.h"
#include "common.h"

/*------------------------------------------
              abstract file system
  ------------------------------------------*/

void filesystem_init(filesystem_t *fs, const char *name,
                     access_handle_t access_handle, open_handle_t open_handle) {
  fs->name = name;
  inode_manager_init(&fs->inode_manager);
  fs->access_handle = access_handle;
  fs->open_handle = open_handle;
}

void filesystem_destroy(filesystem_t *fs) {
  inode_manager_destroy(&fs->inode_manager);
  fs->access_handle = NULL;
  fs->open_handle = NULL;
}

filesystem_t *new_filesystem(const char *name, access_handle_t access_handle,
                             open_handle_t open_handle) {
  filesystem_t *fs = pmm->alloc(sizeof(filesystem_t));
  filesystem_init(fs, name, access_handle, open_handle);
}

void delete_filesystem(filesystem_t *fs) {
  filesystem_destroy(fs);
  pmm->free(fs);
}

/*------------------------------------------
                    kvfs
  ------------------------------------------*/

static ssize_t kvfs_read(file_t *this, char *buf, size_t size) {
  TODO;
  return 0;
}

static ssize_t kvfs_write(file_t *this, const char *buf, size_t size) {
  TODO;
  return 0;
}

static off_t kvfs_lseek(file_t *this, off_t offset, int whence) {
  TODO;
  return 0;
}

static int kvfs_close(file_t *this) {
  TODO;
  return 0;
}

static int kvfs_access(filesystem_t *this, const char *path, int mode) {
  TODO;
  return 0;
}

static int kvfs_open(filesystem_t *this, const char *path, int flags) {
  inode_t *inode = inode_manager_lookup(&this->inode_manager, path, 
    INODE_FILE, (flags & O_CREAT), DEFAULT_MODE);
  if (inode == NULL) {
    LOG("Can't find path %s", path);
    return -1;
  }
  
  int fd = file_table_alloc(inode, kvfs_read, kvfs_write, kvfs_lseek, kvfs_close);

}

filesystem_t *new_kvfs(const char *name) {
  return new_filesystem(name, kvfs_access, kvfs_open);
}