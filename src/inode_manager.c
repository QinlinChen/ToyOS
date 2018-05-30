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
static inode_t *inode_recursive_lookup(inode_t *node, const char *path, int flags) {
  printf("path %s\n", path);
  // if then exit
  char name[MAXPATHLEN];
  int i = 0;
  int is_file = 0;

  // parse path
  Assert(*path == '/');
  path++;
  Assert(*path != '\0');
  while (*path != '\0' && *path != '/')
    name[i++] = *path;
  name[i++] = '\0';
  if  (*path == '\0')
    is_file = 1;

  inode_t *child = inode_find_child(node, name);
  // if found
  if (child != NULL) {
    if (is_file)
      return child;
    return inode_recursive_lookup(child, path, flags);
  }

  // not found
  if (flags & O_CREAT) {
    // create
    int type = (is_file ? INODE_FILE : INODE_DIR);
    inode_t *new_child = new_inode(name, type, DEFAULT_MODE);
    inode_add_child(node, new_child);
    if (is_file)
      return new_child;
    return inode_recursive_lookup(new_child, path, flags);
  }

  // not found and not created
  return NULL;
}

static void inode_recursive_print(inode_t *node, int depth) {
  char r = ((node->mode & S_IRUSR) ? 'r' : '-');
  char w = ((node->mode & S_IWUSR) ? 'w' : '-');
  char x = ((node->mode & S_IXUSR) ? 'x' : '-');
  for (int i = 0; i < depth; ++i)
    printf("    ");
  printf("%s[%c%c%c]\n", node->name, x, w, r);
  for (inode_t *scan = node->child; scan != NULL; scan = scan->sibling)
    inode_recursive_print(scan, depth + 1);
}

void inode_manager_init(inode_manager_t *inode_manager) {
  inode_manager->root = new_inode("/", INODE_DIR, DEFAULT_MODE);
}

inode_t *inode_manager_lookup(inode_manager_t *inode_manager, const char *path, int flags) {
  if (strcmp(path, "/") == 0)
    return inode_manager->root;
  return inode_recursive_lookup(inode_manager->root, path, flags);
}

void inode_manager_print(inode_manager_t *inode_manager) {
  inode_recursive_print(inode_manager->root, 0);
}
