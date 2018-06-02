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
  Assert(fs != NULL);
  filesystem_init(fs, name, ops);
  return fs;
}

void delete_filesystem(filesystem_t *fs) {
  Assert(fs != NULL);
  filesystem_destroy(fs);
  pmm->free(fs);
}

// basic file system's implementation
static ssize_t basic_file_read(file_t *this, void *buf, size_t size) {
  Assert(this != NULL && buf != NULL);
  kmt->spin_lock(&this->lock);

  if (!this->readable) {
    Log("Read permission denied!");
    kmt->spin_unlock(&this->lock);
    return -1;
  }

  ssize_t nread = inode_manager_read(this->inode_manager, this->inode,
                                     this->offset, buf, size);
  this->offset += nread;

  kmt->spin_unlock(&this->lock);
  return nread;
}

static ssize_t basic_file_write(file_t *this, const void *buf, size_t size) {
  kmt->spin_lock(&this->lock);

  if (!this->writable) {
    Log("Write permission denied!");
    kmt->spin_unlock(&this->lock);
    return -1;
  }

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
    Log("Offset is out of bound!");
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
  Assert((mode & ~R_OK & ~W_OK & ~X_OK) == 0);
  kmt->spin_lock(&this->lock);
  inode_manager_t *manager = &this->inode_manager;
  inode_t *inode = inode_manager_lookup(manager, path, INODE_FILE, 0, 0);
  if (inode == NULL) {
    kmt->spin_unlock(&this->lock);
    return 0;
  }
  int ok = inode_manager_checkmode(manager, inode, mode);
  kmt->spin_unlock(&this->lock);
  return ok;
}

static file_t *basic_fs_open(filesystem_t *this, const char *path, int flags, file_ops_t *ops) {
  Assert(this != NULL && path != NULL);
  kmt->spin_lock(&this->lock);
  // get inode
  inode_manager_t *manager = &this->inode_manager;
  inode_t *inode = inode_manager_lookup(manager, path, INODE_FILE,
                                        (flags & O_CREAT), DEFAULT_MODE);
  if (inode == NULL) {
    Log("Can't find path %s", path);
    kmt->spin_unlock(&this->lock);
    return NULL;
  }

  // decide permission
  int readable = 0, writable = 0, mode = 0;
  if ((flags & O_RDONLY) || (flags & O_RDWR)) {
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
    return NULL;
  }

  // allocate fd
  file_t *file = file_table_alloc(inode, manager, readable, writable, ops);
  Assert(file != NULL);
  kmt->spin_unlock(&this->lock);
  return file;
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

static file_t *kvfs_open(filesystem_t *this, const char *path, int flags) {
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
  filesystem_t *fs = new_filesystem(name, &ops);

  // Remember to add /dev and /proc directories before calling mount.
  inode_manager_t *manager = &fs->inode_manager;
  inode_manager_lookup(manager, "/dev", INODE_DIR, 1, S_IRUSR);
  inode_manager_lookup(manager, "/proc", INODE_DIR, 1, S_IRUSR);
  return fs;
}

/*------------------------------------------
                    devfs
  ------------------------------------------*/

static ssize_t devfs_read(file_t *this, void *buf, size_t size) {
  kmt->spin_lock(&this->lock);
  inode_manager_t *manager = this->inode_manager;
  inode_t *inode = this->inode;

  if (!this->readable) {
    Log("Read permission denied!");
    kmt->spin_unlock(&this->lock);
    return -1;
  }

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

  if (!this->writable) {
    Log("Write permission denied!");
    kmt->spin_unlock(&this->lock);
    return -1;
  }

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

static file_t *devfs_open(filesystem_t *this, const char *path, int flags) {
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

filesystem_t *new_devfs(const char *name) {
  filesystem_ops_t ops;
  ops.access_handle = devfs_access;
  ops.open_handle = devfs_open;
  filesystem_t *fs = new_filesystem(name, &ops);
  // devfs has three devices: null, zero and random.
  inode_manager_t *manager = &fs->inode_manager;
  inode_manager_lookup(manager, "/null", INODE_FILE, 1, DEFAULT_MODE);
  inode_manager_lookup(manager, "/zero", INODE_FILE, 1, DEFAULT_MODE);
  inode_manager_lookup(manager, "/random", INODE_FILE, 1, DEFAULT_MODE);
  return fs;
}

/*------------------------------------------
                    procfs
  ------------------------------------------*/

static ssize_t procfs_read(file_t *this, void *buf, size_t size) {
  return basic_file_read(this, buf, size);
}

static ssize_t procfs_write(file_t *this, const void *buf, size_t size) {
  return basic_file_write(this, buf, size);
}

static off_t procfs_lseek(file_t *this, off_t offset, int whence) {
  return basic_file_lseek(this, offset, whence);
}

static int procfs_close(file_t *this) {
  return basic_file_close(this);
}

static int procfs_access(filesystem_t *this, const char *path, int mode) {
  return basic_fs_access(this, path, mode);
}

static file_t *procfs_open(filesystem_t *this, const char *path, int flags) {
  if (flags & O_CREAT) {
    Log("Forbid creating files in procfs");
    return -1;
  }
  file_ops_t ops;
  ops.read_handle = procfs_read;
  ops.write_handle = procfs_write;
  ops.lseek_handle = procfs_lseek;
  ops.close_handle = procfs_close;
  return basic_fs_open(this, path, flags, &ops);
}

void procfs_add_metainfo(filesystem_t *procfs, const char *name,
                         const char *content, size_t size) {
  Assert(strcmp(procfs->name, "procfs") == 0);
  char path[MAXPATHLEN];
  strcpy(path, "/");
  strcat(path, name);

  kmt->spin_lock(&procfs->lock);
  inode_manager_t *manager = &procfs->inode_manager;
  inode_t *inode = inode_manager_lookup(manager, path, INODE_FILE, 1, S_IRUSR);
  size_t nwritten = inode_manager_write(manager, inode, 0, content, strlen(content));
  Assert(nwritten == strlen(content));
  kmt->spin_unlock(&procfs->lock);
}

void procfs_add_procinfo(filesystem_t *procfs, int tid, const char *name,
                         const char *content, size_t size) {
  Assert(strcmp(procfs->name, "procfs") == 0);
  char path[MAXPATHLEN], number[32];
  strcpy(path, "/");
  itoa(tid, 10, 1, number);
  strcat(path, number);
  strcat(path, "/");
  strcat(path, name);

  kmt->spin_lock(&procfs->lock);
  inode_manager_t *manager = &procfs->inode_manager;
  inode_t *inode = inode_manager_lookup(manager, path, INODE_FILE, 1, S_IRUSR);
  size_t nwritten = inode_manager_write(manager, inode, 0, content, strlen(content));
  Assert(nwritten == strlen(content));
  kmt->spin_unlock(&procfs->lock);                          
}

filesystem_t *new_procfs(const char *name) {
  filesystem_ops_t ops;
  ops.access_handle = procfs_access;
  ops.open_handle = procfs_open;
  filesystem_t *fs = new_filesystem(name, &ops);

  // add cpuinfo and meminfo
  const char *cpuinfo = "I am cpu infomation!";
  const char *meminfo = "I am memeory infomation!";
  procfs_add_metainfo(fs, "cpuinfo", cpuinfo, strlen(cpuinfo));
  procfs_add_metainfo(fs, "meminfo", meminfo, strlen(meminfo));

  return fs;
}

/*------------------------------------------
                    stdin
  ------------------------------------------*/

static ssize_t stdin_read(file_t *this, void *buf, size_t size) {
  TODO;
  return basic_file_read(this, buf, size);
}

static ssize_t stdin_write(file_t *this, const void *buf, size_t size) {
  return -1;
}

static off_t stdin_lseek(file_t *this, off_t offset, int whence) {
  return 0;
}

static int stdin_close(file_t *this) {
  return -1;
}

void init_as_stdin(file_t *file) {
  file->offset = file ->ref_count = 0;
  file->inode = NULL;
  file->inode_manager = NULL;
  file->readable = 1;
  file->writable = 0;
  kmt->init(file->lock, "stdin_lock");
  file->ops.read_handle = stdin_read;
  file->ops.write_handle = stdin_write;
  file->ops.lseek_handle = stdin_lseek;
  file->ops.close_handle = stdin_close;
}

/*------------------------------------------
                    stdout
  ------------------------------------------*/

static ssize_t stdout_read(file_t *this, void *buf, size_t size) {
  return -1;
}

static ssize_t stdout_write(file_t *this, const void *buf, size_t size) {
  kmt->spin_lock(&this->lock);
  size_t nwritten = 0;
  const char *bufp = buf;
  while (size > 0) {
    _putc(*bufp++);
    nwritten++;
    size--;
  }
  kmt->spin_unlock(&this->lock);
  return nwritten;
}

static off_t stdout_lseek(file_t *this, off_t offset, int whence) {
  return 0;
}

static int stdout_close(file_t *this) {
  return -1;
}

void init_as_stdout(file_t *file) {
  file->offset = file ->ref_count = 0;
  file->inode = NULL;
  file->inode_manager = NULL;
  file->readable = 0;
  file->writable = 1;
  kmt->init(file->lock, "stdout_lock");
  file->ops.read_handle = stdout_read;
  file->ops.write_handle = stdout_write;
  file->ops.lseek_handle = stdout_lseek;
  file->ops.close_handle = stdout_close;
}

/*------------------------------------------
                    stderr
  ------------------------------------------*/

static ssize_t stderr_read(file_t *this, void *buf, size_t size) {
  return -1;
}

static ssize_t stderr_write(file_t *this, const void *buf, size_t size) {
  kmt->spin_lock(&this->lock);
  size_t nwritten = 0;
  const char *bufp = buf;
  while (size > 0) {
    _putc(*bufp++);
    nwritten++;
    size--;
  }
  kmt->spin_unlock(&this->lock);
  return 0;
}

static off_t stderr_lseek(file_t *this, off_t offset, int whence) {
  return 0;
}

static int stderr_close(file_t *this) {
  return -1;
}

void init_as_stderr(file_t *file) {
  file->offset = file ->ref_count = 0;
  file->inode = NULL;
  file->inode_manager = NULL;
  file->readable = 0;
  file->writable = 1;
  kmt->init(file->lock, "stderr_lock");
  file->ops.read_handle = stderr_read;
  file->ops.write_handle = stderr_write;
  file->ops.lseek_handle = stderr_lseek;
  file->ops.close_handle = stderr_close;
}