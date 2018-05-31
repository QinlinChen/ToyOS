#include "os.h"
#include "common.h"

static void vfs_init();
static int vfs_access(const char *path, int mode);
static int vfs_mount(const char *path, filesystem_t *fs);
static int vfs_unmount(const char *path);
static int vfs_open(const char *path, int flags);
static ssize_t vfs_read(int fd, void *buf, size_t nbyte);
static ssize_t vfs_write(int fd, void *buf, size_t nbyte);
static off_t vfs_lseek(int fd, off_t offset, int whence);
static int vfs_close(int fd);

MOD_DEF(vfs) {
  .init = vfs_init,
  .access = vfs_access,
  .mount = vfs_mount,
  .unmount = vfs_unmount,
  .open = vfs_open,
  .read = vfs_read,
  .write = vfs_write,
  .lseek = vfs_lseek,
  .close = vfs_close,
};

/*------------------------------------------
                    vfs
  ------------------------------------------*/

static void vfs_init() {
  // fs_manager_init();
  // fs_manager_add("/", new_kvfs("kvfs"));
  // fs_manager_add("/proc", new_procfs("procfs"));
  // fs_manager_add("/dev", new_devfs("devfs"));
  TODO;
}

static int vfs_access(const char *path, int mode) {
  char subpath[MAXPATHLEN];
  filesystem_t *fs = fs_manager_get(path, subpath);
  if (fs == NULL)
    TODO;
  // fs->lookup(fs, subpath, 0);
  TODO;
  return 0;
}

static int vfs_mount(const char *path, filesystem_t *fs) {
  TODO;
  return fs_manager_add(path, fs);
}

static int vfs_unmount(const char *path) {
  TODO;
  return fs_manager_remove(path);
}

static int vfs_open(const char *path, int flags) {
  char subpath[MAXPATHLEN];
  filesystem_t *fs = fs_manager_get(path, subpath);
  if (fs == NULL) {
    Panic("Illegal path");
    return -1;
  }

  // Assert(fs->lookup != NULL);
  // inode_t *inode = fs->lookup(fs, subpath, flags);
  TODO;
  return 0;
}

static ssize_t vfs_read(int fd, void *buf, size_t nbyte) {
  TODO;
  return 0;
}

static ssize_t vfs_write(int fd, void *buf, size_t nbyte) {
  TODO;
  return 0;
}

static off_t vfs_lseek(int fd, off_t offset, int whence) {
  TODO;
  return 0;
}

static int vfs_close(int fd) {
  TODO;
  return 0;
}
