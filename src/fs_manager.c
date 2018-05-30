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

void fs_manager_init() {
  fs_manager.head = NULL;
}

int fs_manager_add(const char *path, filesystem_t *fs) {
  fs_manager_node_t *node = pmm->alloc(sizeof(fs_manager_node_t));
  if (node == NULL) {
    Panic("Fail to add file system");
    return -1;
  }

  strcpy(node->path, path);
  size_t len = strlen(node->path);
  if (strcmp(node->path, "/") != 0 && node->path[len - 1] == '/')
    node->path[len - 1] = '\0';   // trim the tail '/' if exists
  node->fs = fs;
  node->next = fs_manager.head;
  if (fs_manager.head != NULL)
    fs_manager.head->prev = node;
  fs_manager.head = node;
  node->prev = NULL;
  return 0;
}

filesystem_t *fs_manager_get(const char *path, char *subpath) {
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
      if (path[i] != '/')
        subpath[j++] = '/';
      strcpy(subpath + j, path + i);
      return cur->fs;
    }
  }
  return NULL;
}

int fs_manager_remove(const char *path) {
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
  return -1;
}

void fs_manager_print() {
  for (fs_manager_node_t *cur = fs_manager.head; cur != NULL; cur = cur->next)
    printf("fs: %s, mounted path: %s\n", cur->fs->name, cur->path);
}