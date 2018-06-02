#include "os.h"
#include "common.h"
#include "klib.h"

#define NR_FS 16

typedef struct fs_manager_node {
  char path[MAXPATHLEN];
  filesystem_t *fs;
  struct fs_manager_node *prev;
  struct fs_manager_node *next;
} fs_manager_node_t;

// implemented as list
typedef struct fs_manager {
  fs_manager_node_t *head;
} fs_manager_t;

static fs_manager_t fs_manager;
static spinlock_t lock = SPINLOCK_INIT("fs_manager_lock");

void fs_manager_init() {
  kmt->spin_lock(&lock);
  fs_manager.head = NULL;
  kmt->spin_unlock(&lock);
}

int fs_manager_add(const char *path, filesystem_t *fs) {
  Assert(path != NULL);
  Assert(fs != NULL);
  // allocate node
  fs_manager_node_t *node = pmm->alloc(sizeof(fs_manager_node_t));
  if (node == NULL) {
    Panic("Fail to add file system");
    return -1;
  }

  // construct node
  strcpy(node->path, path);
  size_t len = strlen(node->path);
  if (strcmp(node->path, "/") != 0 && node->path[len - 1] == '/')
    node->path[len - 1] = '\0';   // trim the tail '/' if exists
  node->fs = fs;

  // add node to fs_manager
  kmt->spin_lock(&lock);
  node->next = fs_manager.head;
  if (fs_manager.head != NULL)
    fs_manager.head->prev = node;
  fs_manager.head = node;
  node->prev = NULL;
  kmt->spin_unlock(&lock);

  return 0;
}

filesystem_t *fs_manager_get(const char *path, char *subpath) {
  Assert(path != NULL);
  kmt->spin_lock(&lock);
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
      if (subpath != NULL) {
        j = 0;
        if (path[i] != '/')
          subpath[j++] = '/';
        strcpy(subpath + j, path + i);
      }
      kmt->spin_unlock(&lock);
      return cur->fs;
    }
  }
  kmt->spin_unlock(&lock);
  return NULL;
}

filesystem_t *fs_manager_remove(const char *path) {
  Assert(path != NULL);
  kmt->spin_lock(&lock);
  for (fs_manager_node_t *cur = fs_manager.head; cur != NULL; cur = cur->next)
    if (strcmp(cur->path, path) == 0) {
      if (cur->prev != NULL)
        cur->prev->next = cur->next;
      else
        fs_manager.head = cur->next;
      if (cur->next != NULL)
        cur->next->prev = cur->prev;
      filesystem_t *ret = cur->fs;
      pmm->free(cur);
      kmt->spin_unlock(&lock);
      return ret;
    }
  kmt->spin_unlock(&lock);
  return NULL;
}

void fs_manager_print() {
  kmt->spin_lock(&lock);
  for (fs_manager_node_t *cur = fs_manager.head; cur != NULL; cur = cur->next)
    printf("fs: %s, mounted path: %s\n", cur->fs->name, cur->path);
  kmt->spin_unlock(&lock);
}