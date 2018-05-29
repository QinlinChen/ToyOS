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
                  fs_manager
  ------------------------------------------*/

#define NR_FS 16

typedef struct fs_manager_node {
  char path[MAXPATHLEN];
  filesystem_t *fs;
  struct fs_manager_node *prev;
  struct fs_manager_node *next;
} fs_manager_node_t;

typedef struct fs_manager {
  fs_manager_node_t *head;
} fs_manager_t;

static fs_manager_t fs_manager;

void filesystem_manager_init() {
  fs_manager.head = NULL;
}

int filesystem_manager_add(const char *path, filesystem_t *fs) {
  fs_manager_node_t *node = pmm->alloc(sizeof(fs_manager_node_t));
  if (node == NULL) {
    Panic("Fail to allocate fs_manager_node");
    return -1;
  }
  strcpy(node->path, path);
  node->fs = fs;
  node->next = fs_manager.head;
  if (fs_manager.head != NULL)
    fs_manager.head->prev = node;
  fs_manager.head = node;
  node->prev = NULL;
  return 0;
}

filesystem_t *filesystem_manager_get(const char *path, char *subpath) {
  for (fs_manager_node_t *cur = fs_manager.head; cur != NULL; cur = cur->next) {
    char *mount_point = cur->path;
    int is_found = 1;
    int i, j;
    for (i = 0; mount_point[i] != '\0'; ++i)
      if (mount_point[i] != path[i]) {
        is_found = 0;
        break;
      }
    if (is_found) {
      j = 0;
      for (; path[i] != '\0'; ++i, ++j)
        subpath[j] = path[i];
      subpath[j] = '\0';
      return cur->fs;
    }
  }
  Panic("filesystem manager can't match the path with a mounted fs");
  return NULL;
}

int filesystem_manager_remove(const char *path) {
  for (fs_manager_node_t *cur = fs_manager.head; cur != NULL; cur = cur->next)
    if (strcmp(cur->path, path) == 0) {
      if (cur->prev != NULL)
        cur->prev->next = cur->next;
      else
        fs_manager.head = cur->next;
      if (cur->next != NULL)
        cur->next->prev = cur->prev;
      return 0;
    }
  Panic("filesystem manager underflows");
  return -1;
}

void filesystem_manager_print() {
  for (fs_manager_node_t *cur = fs_manager.head; cur != NULL; cur = cur->next)
    printf("fs: %s, mounted path: %s\n", cur->fs->name, cur->path);
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
