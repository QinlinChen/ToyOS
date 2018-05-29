#include "os.h"
#include "debug.h"

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
              filesystem_manager
  ------------------------------------------*/

#define NR_FS 2

typedef struct filesystem_manager {
  char path[MAXPATHLEN][NR_FS];
  filesystem_t *fs[NR_FS];
  int is_free[NR_FS];
} filesystem_manager_t;

static filesystem_manager_t fs_manager;

void filesystem_manager_init() {
  for (int i = 0; i < NR_FS; ++i) {
    fs_manager.fs[i] = NULL;
    fs_manager.is_free[i] = 1;
  }
}

int filesystem_manager_add(const char *path, filesystem_t *fs) {
  for (int i = 0; i < NR_FS; ++i)
    if (fs_manager.is_free[i]) {
      strcpy(fs_manager.path[i], path);
      fs_manager.fs[i] = fs;
      fs_manager.is_free[i] = 0;
      return 1;
    } 
  Panic("filesystem manager overflows");
  return 0;
}

filesystem_t *filesystem_manager_get(const char *path, char *subpath) {
  for (int n = 0; n < NR_FS; ++n)
    if (!fs_manager.is_free[n]) {
      int is_found = 1;
      int i, j;
      for (i = 0; fs_manager.path[n][i] != '\0'; ++i)
        if (fs_manager.path[n][i] != path[i]) {
          is_found = 0;
          break;
        }
      if (is_found) {
        j = 0;
        for (; path[i] != '\0'; ++i, ++j)
          subpath[j] = path[i];
        subpath[j] = '\0';
        return fs_manager.fs[n];
      }
    }
  Panic("filesystem manager can't match the path with a mounted fs");
  return NULL;
}

int filesystem_manager_remove(const char *path) {
  for (int i = 0; i < NR_FS; ++i)
    if (!fs_manager.is_free[i] && strcmp(fs_manager.path[i], path) == 0) {
      fs_manager.fs[i] = NULL;
      fs_manager.is_free[i] = 1;
      return 1;
    }
  Panic("filesystem manager underflows");
  return 0;
}

void filesystem_manager_print() {
  for (int i = 0; i < NR_FS; ++i) {
    printf("fs: %s, mounted path: %s\n", 
      fs_manager.fs[i]->name, fs_manager.path[i]);
  }
  return 0;
}

/*------------------------------------------
                    vfs
  ------------------------------------------*/

static void vfs_init() {
  TODO;
}

static int vfs_access(const char *path, int mode) {
  TODO;
  return 0;
}

static int vfs_mount(const char *path, filesystem_t *fs) {
  TODO;
  return 0;
}

static int vfs_unmount(const char *path) {
  TODO;
  return 0;
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
