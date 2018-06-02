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
  kmt->spin_unlock(&lock);
}

int file_table_alloc(inode_t *inode, read_handle_t read_handle, write_handle_t write_handle,
                     lseek_handle_t lseek_handle, close_handle_t close_handle) {
  Assert(inode != NULL);
  kmt->spin_lock(&lock);
  for (int fd = 0; fd < NR_FD; ++fd)
    if (is_free[fd]) {
      file_t *file = &file_table[fd];
      file->offset = 0;
      file->inode = inode;
      file->ref_count = 1;
      kmt->spin_init(&file->lock, "file_lock");
      file->read_handle = read_handle;
      file->write_handle = write_handle;
      file->lseek_handle = lseek_handle;
      file->close_handle = close_handle;
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
  file->read_handle = NULL;
  file->write_handle = NULL;
  file->lseek_handle = NULL;
  file->close_handle = NULL;
  file->inode = NULL;
  is_free[file - file_table] = 1;
  kmt->spin_unlock(&lock);
}

file_t *file_table_get(int fd) {
  kmt->spin_lock(&lock);
  file_t *file = &file_table[fd];
  kmt->spin_unlock(&lock);
  return file;
}