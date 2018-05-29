#include "os.h"
#include "debug.h"

static void vfs_init();
static void vfs_access(const char *path, int mode);
static void vfs_mount(const char *path, filesystem_t *fs);
static void vfs_unmount(const char *path);
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

static void vfs_init() {
  TODO;
}

static void vfs_access(const char *path, int mode) {
  TODO;
}

static void vfs_mount(const char *path, filesystem_t *fs) {
  TODO;
}

static void vfs_unmount(const char *path) {
  TODO;
}

static int vfs_open(const char *path, int flags) {
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
