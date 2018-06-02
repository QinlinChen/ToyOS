#include "os.h"
#include "common.h"

/*------------------------------------------
              basic file system
  ------------------------------------------*/

void filesystem_init(filesystem_t *fs, const char *name, filesystem_ops_t *ops) {
  Assert(fs != NULL && ops != NULL);
  fs->name = name;
  inode_manager_init(&fs->inode_manager);
  fs->ops = *ops;
  kmt->spin_init(&fs->lock, "filesystem_lock");
}

void filesystem_destroy(filesystem_t *fs) {
  Assert(fs != NULL);
  kmt->spin_lock(&fs->lock);
  inode_manager_destroy(&fs->inode_manager);
  fs->ops.access_handle = NULL;
  fs->ops.open_handle = NULL;
  kmt->spin_unlock(&fs->lock);
}

filesystem_t *new_filesystem(const char *name, filesystem_ops_t *ops) {
  filesystem_t *fs = pmm->alloc(sizeof(filesystem_t));
  filesystem_init(fs, name, ops);
  return fs;
}

void delete_filesystem(filesystem_t *fs) {
  filesystem_destroy(fs);
  pmm->free(fs);
}

// basic file system's implementation
static ssize_t basic_file_read(file_t *this, void *buf, size_t size) {
  kmt->spin_lock(&this->lock);
  ssize_t nread = inode_manager_read(this->inode_manager, this->inode,
                                     this->offset, buf, size);
  this->offset += nread;
  kmt->spin_unlock(&this->lock);
  return nread;
}

static ssize_t basic_file_write(file_t *this, const void *buf, size_t size) {
  kmt->spin_lock(&this->lock);
  ssize_t nwritten = inode_manager_write(this->inode_manager, this->inode,
                                         this->offset, buf, size);
  this->offset += nwritten;
  kmt->spin_unlock(&this->lock);
  return nwritten;
}

static off_t basic_file_lseek(file_t *this, off_t offset, int whence) {
  kmt->spin_lock(&this->lock);
  size_t filesize = inode_manager_get_filesize(this->inode_manager, this->inode);
  switch (whence) {
    case SEEK_SET: break;
    case SEEK_CUR: offset += this->offset; break;
    case SEEK_END: offset += filesize; break;
    default: Panic("Should not reach here");
  }
  if (offset > filesize || offset < 0) {
    kmt->spin_unlock(&this->lock);
    return -1;
  }
  this->offset = offset;
  kmt->spin_unlock(&this->lock);
  return offset;
}

static int basic_file_close(file_t *this) {
  kmt->spin_lock(&this->lock);
  this->ref_count--;
  if (this->ref_count == 0)
    file_table_free(this);
  kmt->spin_unlock(&this->lock);
  return 0;
}

static int basic_fs_access(filesystem_t *this, const char *path, int mode) {
  Assert(this != NULL && path != NULL);
  Assert(((mode & ~F_OK) == 0) || (mode & ~R_OK & ~W_OK & ~X_OK) == 0);
  kmt->spin_lock(&this->lock);
  inode_manager_t *manager = &this->inode_manager;
  inode_t *inode = inode_manager_lookup(manager, path, INODE_FILE, 0, 0);
  if (inode == NULL) {
    kmt->spin_unlock(&this->lock);
    return 0;
  }
  if (mode & F_OK) {
    kmt->spin_unlock(&this->lock);
    return 1;
  }
  int ok = inode_manager_checkmode(manager, inode, mode);
  kmt->spin_unlock(&this->lock);
  return ok;
}

static int basic_fs_open(filesystem_t *this, const char *path, int flags, file_ops_t *ops) {
  Assert(this != NULL && path != NULL);
  kmt->spin_lock(&this->lock);
  // get inode
  inode_manager_t *manager = &this->inode_manager;
  inode_t *inode = inode_manager_lookup(manager, path, INODE_FILE,
                                        (flags & O_CREAT), DEFAULT_MODE);
  if (inode == NULL) {
    Log("Can't find path %s", path);
    kmt->spin_unlock(&this->lock);
    return -1;
  }

  // decide permission
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
    kmt->spin_unlock(&this->lock);
    return -1;
  }

  // allocate fd
  int fd = file_table_alloc(inode, manager, readable, writable, ops);
  kmt->spin_unlock(&this->lock);
  return fd;
}

/*------------------------------------------
                    kvfs
  ------------------------------------------*/

static ssize_t kvfs_read(file_t *this, void *buf, size_t size) {
  return basic_file_read(this, buf, size);
}

static ssize_t kvfs_write(file_t *this, const void *buf, size_t size) {
  return basic_file_write(this, buf, size);
}

static off_t kvfs_lseek(file_t *this, off_t offset, int whence) {
  return basic_file_lseek(this, offset, whence);
}

static int kvfs_close(file_t *this) {
  return basic_file_close(this);
}

static int kvfs_access(filesystem_t *this, const char *path, int mode) {
  return basic_fs_access(this, path, mode);
}

static int kvfs_open(filesystem_t *this, const char *path, int flags) {
  file_ops_t ops;
  ops.read_handle = kvfs_read;
  ops.write_handle = kvfs_write;
  ops.lseek_handle = kvfs_lseek;
  ops.close_handle = kvfs_close;
  return basic_fs_open(this, path, flags, &ops);
}

filesystem_t *new_kvfs(const char *name) {
  filesystem_ops_t ops;
  ops.access_handle = kvfs_access;
  ops.open_handle = kvfs_open;
  return new_filesystem(name, &ops);
}

/*------------------------------------------
                    devfs
  ------------------------------------------*/

static ssize_t devfs_read(file_t *this, void *buf, size_t size) {
  kmt->spin_lock(&this->lock);
  inode_manager_t *manager = this->inode_manager;
  inode_t *inode = this->inode;

  // read null
  if (inode_manager_cmp_name(manager, inode, "null") == 0) {
    kmt->spin_unlock(&this->lock); 
    return 0;
  }

  // read zero
  if (inode_manager_cmp_name(manager, inode, "zero") == 0) {
    size_t nread;
    for (nread = 0; nread < size; ++nread)
      ((char *)buf)[nread] = 0;
    kmt->spin_unlock(&this->lock); 
    return nread;
  }

  // read random
  if (inode_manager_cmp_name(manager, inode, "random") == 0) {
    size_t nread;
    for (nread = 0; nread < size; ++nread)
      ((char *)buf)[nread] = (rand() % (1 << 8));
    kmt->spin_unlock(&this->lock); 
    return nread;
  }

  Panic("Should not reach here!");
  kmt->spin_unlock(&this->lock); 
  return -1;
}

static ssize_t devfs_write(file_t *this, const void *buf, size_t size) {
  kmt->spin_lock(&this->lock);
  inode_manager_t *manager = this->inode_manager;
  inode_t *inode = this->inode;

  // write null
  if (inode_manager_cmp_name(manager, inode, "null") == 0) {
    kmt->spin_unlock(&this->lock); 
    return size;
  }

  // write zero
  if (inode_manager_cmp_name(manager, inode, "zero") == 0) {
    kmt->spin_unlock(&this->lock); 
    return 0;
  }

  // write random
  if (inode_manager_cmp_name(manager, inode, "random") == 0) {
    kmt->spin_unlock(&this->lock); 
    return 0;
  }

  Panic("Should not reach here!");
  kmt->spin_unlock(&this->lock); 
  return -1;
}

static off_t devfs_lseek(file_t *this, off_t offset, int whence) {
  return 0;
}

static int devfs_close(file_t *this) {
  return basic_file_close(this);
}

static int devfs_access(filesystem_t *this, const char *path, int mode) {
  return basic_fs_access(this, path, mode);
}

static int devfs_open(filesystem_t *this, const char *path, int flags) {
  if (flags & O_CREAT) {
    Log("Forbid creating files in devfs");
    return -1;
  }
  file_ops_t ops;
  ops.read_handle = devfs_read;
  ops.write_handle = devfs_write;
  ops.lseek_handle = devfs_lseek;
  ops.close_handle = devfs_close;
  return basic_fs_open(this, path, flags, &ops);
}

filesystem_t *new_kvfs(const char *name) {
  filesystem_ops_t ops;
  ops.access_handle = devfs_access;
  ops.open_handle = devfs_open;
  filesystem_t *fs = new_filesystem(name, &ops);
  inode_manager_lookup(&fs->inode_manager, "/null", INODE_FILE, 1, DEFAULT_MODE);
  inode_manager_lookup(&fs->inode_manager, "/zero", INODE_FILE, 1, DEFAULT_MODE);
  inode_manager_lookup(&fs->inode_manager, "/random", INODE_FILE, 1, DEFAULT_MODE);
  return fs;
}
