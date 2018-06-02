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
  file_table_init();
  fs_manager_init();
  fs_manager_add("/", new_kvfs("kvfs"));
  // fs_manager_add("/proc", new_procfs("procfs"));
  // fs_manager_add("/dev", new_devfs("devfs"));
  Log("vfs initialized");
}

static int vfs_mount(const char *path, filesystem_t *fs) {
  fs_manager_add(path, fs);
  return 0;
}

static int vfs_unmount(const char *path) {
  filesystem_t *fs = fs_manager_remove(path);
  if (fs != NULL)
    delete_filesystem(fs);
  return 0;
}

static int vfs_access(const char *path, int mode) {
  char subpath[MAXPATHLEN];
  filesystem_t *fs = fs_manager_get(path, subpath);
  if (fs == NULL)
    return -1;
  Assert(fs->access_handle != NULL);
  return fs->access_handle(fs, subpath, mode);
}

static int vfs_open(const char *path, int flags) {
  char subpath[MAXPATHLEN];
  filesystem_t *fs = fs_manager_get(path, subpath);
  if (fs == NULL)
    return -1;
  Assert(fs->open_handle != NULL);
  return fs->open_handle(fs, subpath, flags);
}

static ssize_t vfs_read(int fd, void *buf, size_t size) {
  file_t *file = file_table_get(fd);
  Assert(file->read_handle != NULL);
  return file->read_handle(file, buf, size);
}

static ssize_t vfs_write(int fd, void *buf, size_t size) {
  file_t *file = file_table_get(fd);
  Assert(file->write_handle != NULL);
  return file->write_handle(file, buf, size);
}

static off_t vfs_lseek(int fd, off_t offset, int whence) {
  file_t *file = file_table_get(fd);
  Assert(file->lseek_handle != NULL);
  return file->lseek_handle(file, offset, whence);
}

static int vfs_close(int fd) {
  file_t *file = file_table_get(fd);
  Assert(file->close_handle != NULL);
  return file->close_handle(file);
}
