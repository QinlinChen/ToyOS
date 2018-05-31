#include "os.h"
#include "common.h"

static inode_t *new_inode(const char *name, int type, int mode) {
  inode_t *inode = pmm->alloc(sizeof(inode_t));
  Assert(inode != NULL);
  strcpy(inode->name, name);
  inode->type = type;
  inode->size = 0;
  inode->mode = mode;
  inode->parent = inode->child = inode->sibling = NULL;
  return inode;
}

static void delete_inode(inode_t *node) {
  inode_t *scan = node->child;
  while (scan != NULL) {
    inode_t *save = scan->sibling;
    delete_inode(scan);
    scan = save;
  }
  pmm->free(node);
  node->parent = node->child = node->sibling = NULL;
}

static void inode_add_child(inode_t *parent, inode_t *node) {
  node->sibling = parent->child;
  node->child = NULL;
  node->parent = parent;
  parent->child = node;
}

static inode_t *inode_find_child(inode_t *node, const char *name) {
  for (inode_t *scan = node->child; scan != NULL; scan = scan->sibling)
    if (strcmp(scan->name, name) == 0)
      return scan;
  return NULL;
}

// path is not allowed to be root '/'
static inode_t *inode_recursive_lookup(inode_t *node, const char *path, int create, int type) {
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

  inode_t *child = inode_find_child(node, name);
  // if found
  if (child != NULL)
    return is_leaf ? child : inode_recursive_lookup(child, path, create, type);

  // not found but create
  if (create) {
    inode_t *new_child = new_inode(name, (is_leaf ? type : INODE_DIR), DEFAULT_MODE);
    inode_add_child(node, new_child);
    return is_leaf ? new_child : inode_recursive_lookup(new_child, path, create, type);
  }

  // not found and not created
  return NULL;
}

static inode_t *inode_lookup(inode_t *root, const char *path, int create, int type) {
  if (strcmp(path, "/") == 0)
    return type == INODE_DIR ? root : NULL;
  return inode_recursive_lookup(root, path, create, type);
}

static void inode_recursive_print(inode_t *node, int depth) {
  char d = ((node->type == INODE_DIR) ? 'd' : '-');
  char r = ((node->mode & S_IRUSR) ? 'r' : '-');
  char w = ((node->mode & S_IWUSR) ? 'w' : '-');
  char x = ((node->mode & S_IXUSR) ? 'x' : '-');
  for (int i = 0; i < depth; ++i)
    printf("    ");
  printf("%s[%c%c%c%c]\n", node->name, d, x, w, r);
  for (inode_t *scan = node->child; scan != NULL; scan = scan->sibling)
    inode_recursive_print(scan, depth + 1);
}

void inode_manager_init(inode_manager_t *inode_manager) {
  inode_manager->root = new_inode("/", INODE_DIR, DEFAULT_MODE);
}

void inode_manager_destroy(inode_manager_t *inode_manager) {
  delete_inode(inode_manager->root);
  inode_manager->root = NULL;
}

inode_t *inode_manager_lookup(inode_manager_t *inode_manager,
                              const char *path, int create, int type) {
  return inode_lookup(inode_manager->root, path, create, type);
}

void inode_manager_print(inode_manager_t *inode_manager) {
  inode_recursive_print(inode_manager->root, 0);
}
