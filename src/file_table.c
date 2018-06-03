#include "os.h"
#include "common.h"

#define NR_FD 1000

static file_t file_table[NR_FD];
static int is_free[NR_FD];
static spinlock_t lock = SPINLOCK_INIT("file_table_lock");

void file_table_init() {
  kmt->spin_lock(&lock);
  for (int i = 0; i < NR_FD; ++i)
    is_free[i] = 1;
  is_free[STDIN_FILENO] = 0;
  is_free[STDOUT_FILENO] = 0;
  is_free[STDERR_FILENO] = 0;
  init_as_stdin(&file_table[STDIN_FILENO]);
  init_as_stdout(&file_table[STDOUT_FILENO]);
  init_as_stderr(&file_table[STDERR_FILENO]);
  kmt->spin_unlock(&lock);
}

int file_table_alloc(inode_t *inode, inode_manager_t *inode_manager,
                     int readable, int writable, file_ops_t *ops) {
  Assert(ops != NULL);
  kmt->spin_lock(&lock);
  for (int fd = 0; fd < NR_FD; ++fd)
    if (is_free[fd]) {
      file_t *file = &file_table[fd];
      file->offset = 0;
      file->inode = inode;
      file->inode_manager = inode_manager;
      file->ref_count = 1;
      file->writable = (writable ? 1 : 0);
      file->readable = (readable ? 1 : 0);
      kmt->spin_init(&file->lock, "file_lock");
      file->ops = *ops;
      is_free[fd] = 0;
      kmt->spin_unlock(&lock);
      return fd;
    }
  Panic("File table is full");
  kmt->spin_unlock(&lock);
  return -1;
}

void file_table_free(file_t *file) {
  Assert(file != NULL);
  kmt->spin_lock(&lock);
  file->ops.read_handle = NULL;
  file->ops.write_handle = NULL;
  file->ops.lseek_handle = NULL;
  file->ops.close_handle = NULL;
  file->inode = NULL;
  is_free[file - file_table] = 1;
  kmt->spin_unlock(&lock);
}

file_t *file_table_get(int fd) {
  kmt->spin_lock(&lock);
  file_t *file = NULL;
  if (!is_free[fd])
    file = &file_table[fd];
  kmt->spin_unlock(&lock);
  return file;
}