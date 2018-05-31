#include "os.h"
#include "common.h"

static inode_t *new_inode(const char *name, int type, int mode) {
  inode_t *node = pmm->alloc(sizeof(inode_t));
  Assert(node != NULL);
  strcpy(node->name, name);
  node->type = type;
  node->mode = mode;
  node->parent = node->child = node->next = node->prev = NULL;
  string_init(&node->data);
  return node;
}

static void delete_inode(inode_t *node) {
  inode_t *scan = node->child;
  while (scan != NULL) {
    inode_t *save = scan->next;
    delete_inode(scan);
    scan = save;
  }
  node->parent = node->child = node->next = node->prev = NULL;
  string_destroy(&node->data);
  pmm->free(node);
}

static void inode_add_child(inode_t *parent, inode_t *node) {
  node->next = parent->child;
  if (parent->child != NULL)
    parent->child->prev = node;
  parent->child = node;
  node->prev = NULL;
  node->parent = parent;
  node->child = NULL;
}

static void inode_remove(inode_t *node) {
  Assert(node != NULL);
  Assert(node->parent != NULL);
  if (node->prev != NULL)
    node->prev->next = node->next;
  else
    node->parent->child = node->next;
  if (node->next != NULL)
    node->next->prev = node->prev;
  node->parent = node->prev = node->next = NULL;
}

static inode_t *inode_find_child(inode_t *node, const char *name, int type) {
  for (inode_t *scan = node->child; scan != NULL; scan = scan->next)
    if (strcmp(scan->name, name) == 0 && scan->type == type)
      return scan;
  return NULL;
}

// path is not allowed to be root '/'
static inode_t *inode_recursive_lookup(inode_t *node, const char *path, 
                                       int type, int create, int mode) {
  // if then exit
  char name[MAXPATHLEN];
  int i = 0;
  int is_leaf = 0;

  // parse path
  Assert(*path == '/');
  path++;
  Assert(*path != '\0');
  while (*path != '\0' && *path != '/')
    name[i++] = *path++;
  name[i++] = '\0';
  if  (*path == '\0')
    is_leaf = 1;

  // recursive lookup
  inode_t *child;
  if (is_leaf) {
    if ((child = inode_find_child(node, name, type)) != NULL)
      return child;

    if (create) {
      inode_t *new_child = new_inode(name, type, mode);
      inode_add_child(node, new_child);
      return new_child;
    }
  }

  else {
    if ((child = inode_find_child(node, name, INODE_DIR)) != NULL)
      return inode_recursive_lookup(child, path, type, create, mode);
    
    if (create) {
      inode_t *new_child = new_inode(name, INODE_DIR, DEFAULT_MODE);
      inode_add_child(node, new_child);
      return inode_recursive_lookup(new_child, path, type, create, mode);
    }
  }

  // not found and not create
  return NULL;
}

static inode_t *inode_lookup(inode_t *root, const char *path,
                             int type, int create, int mode) {
  if (strcmp(path, "/") == 0)
    return type == INODE_DIR ? root : NULL;
  return inode_recursive_lookup(root, path, type, create, mode);
}

static void inode_recursive_print(inode_t *node, int depth) {
  char d = ((node->type == INODE_DIR) ? 'd' : '-');
  char r = ((node->mode & S_IRUSR) ? 'r' : '-');
  char w = ((node->mode & S_IWUSR) ? 'w' : '-');
  char x = ((node->mode & S_IXUSR) ? 'x' : '-');
  for (int i = 0; i < depth; ++i)
    printf("    ");
  printf("%s[%c%c%c%c]\n", node->name, d, x, w, r);
  for (inode_t *scan = node->child; scan != NULL; scan = scan->next)
    inode_recursive_print(scan, depth + 1);
}

void inode_manager_init(inode_manager_t *inode_manager) {
  inode_manager->root = new_inode("/", INODE_DIR, DEFAULT_MODE);
}

void inode_manager_destroy(inode_manager_t *inode_manager) {
  delete_inode(inode_manager->root);
  inode_manager->root = NULL;
}

inode_t *inode_manager_lookup(inode_manager_t *inode_manager, const char *path, 
                              int type, int create, int mode) {
  return inode_lookup(inode_manager->root, path, type, create, mode);
}

void inode_manager_remove(inode_manager_t *inode_manager, inode_t *inode) {
  inode_remove(inode);
  delete_inode(inode);
}

void inode_manager_print(inode_manager_t *inode_manager) {
  inode_recursive_print(inode_manager->root, 0);
}
