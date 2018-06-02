#include "os.h"
#include "common.h"

/*------------------------------------------
              basic file system
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
  return fs;
}

void delete_filesystem(filesystem_t *fs) {
  filesystem_destroy(fs);
  pmm->free(fs);
}

/*------------------------------------------
                    kvfs
  ------------------------------------------*/

static ssize_t kvfs_read(file_t *this, char *buf, size_t size) {
  kmt->spin_lock(&this->lock);
  string_t *data = &this->inode->data;
  off_t offset = this->offset;
  ssize_t nread = string_read(data, offset, buf, size);
  this->offset += nread;
  kmt->spin_unlock(&this->lock);
  return nread;
}

static ssize_t kvfs_write(file_t *this, const char *buf, size_t size) {
  kmt->spin_lock(&this->lock);
  string_t *data = &this->inode->data;
  off_t offset = this->offset;
  ssize_t nwritten = string_write(data, offset, buf, size);
  this->offset += nwritten;
  kmt->spin_unlock(&this->lock);
  return nwritten;
}

static off_t kvfs_lseek(file_t *this, off_t offset, int whence) {
  kmt->spin_lock(&this->lock);
  string_t *data = &this->inode->data;
  switch (whence) {
    case SEEK_SET: break;
    case SEEK_CUR: offset += this->offset; break;
    case SEEK_END: offset += (off_t)string_length(data); break;
    default: Panic("Should not reach here");
  }
  if (offset > string_length(data) || offset < 0) {
    kmt->spin_unlock(&this->lock);
    return -1;
  }
  this->offset = offset;
  kmt->spin_unlock(&this->lock);
  return offset;
}

static int kvfs_close(file_t *this) {
  kmt->spin_lock(&this->lock);
  this->ref_count--;
  if (this->ref_count == 0)
    file_table_free(this);
  kmt->spin_unlock(&this->lock);
  return 0;
}

static int kvfs_access(filesystem_t *this, const char *path, int mode) {
  Assert(this != NULL && path != NULL);
  Assert(((mode & ~F_OK) == 0) || (mode & ~R_OK & ~W_OK & ~X_OK) == 0);
  inode_manager_t *manager = &this->inode_manager;
  inode_t *inode = inode_manager_lookup(manager, path, INODE_FILE, 0, 0);
  if (inode == NULL)
    return 0;
  if (mode & F_OK)
    return 1;
  return inode_manager_checkmode(manager, inode, mode);
}

static int kvfs_open(filesystem_t *this, const char *path, int flags) {
  Assert(this != NULL && path != NULL);
  inode_manager_t *manager = &this->inode_manager;
  inode_t *inode = inode_manager_lookup(manager, path, INODE_FILE,
                                        (flags & O_CREAT), DEFAULT_MODE);
  if (inode == NULL) {
    Log("Can't find path %s", path);
    return -1;
  }

  int readable = 0, writable = 0, mode = 0;
  if ((flags & O_RONLY) || (flags & O_RDWR)) {
    readable = 1;
    mode |= R_OK;
  }
  if ((flags & O_WRONLY) || (flags & O_RDWR)) {
    writable = 1;
    mode |= W_OK;
  }  
  if (!inode_manager_checkmode(manager, inode, mode)) {
    Log("Permission denied!");
    return -1;
  }

  int fd = file_table_alloc(inode, kvfs_read, kvfs_write, kvfs_lseek, kvfs_close);
  file_t *file = file_table_get(fd);
  kmt->spin_lock(&file->lock);
  file->readable = readable;
  file->writable = writable;
  kmt->spin_unlock(&file->lock);

  return fd;
}

filesystem_t *new_kvfs(const char *name) {
  return new_filesystem(name, kvfs_access, kvfs_open);
}