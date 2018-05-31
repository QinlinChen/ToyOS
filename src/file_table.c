#include "os.h"
#include "common.h"

#define NR_FD 1000

static file_t file_table[NR_FD];
static int is_free[NR_FD];

void file_table_init() {
  for (int i = 0; i < NR_FD; ++i)
    is_free[i] = 1;
  is_free[STDIN_FILENO] = 0;
  is_free[STDOUT_FILENO] = 0;
  is_free[STDERR_FILENO] = 0;
}

int file_table_alloc(inode_manager_t *inode_manager, inode_t *inode, 
                     read_handle_t read_handle, write_handle_t write_handle,
                     lseek_handle_t lseek_handle, close_handle_t close_handle) {
  Assert(inode_manager != NULL && inode != NULL);
  for (int fd = 0; fd < NR_FD; ++fd)
    if (is_free[fd]) {
      file_t *file = &file_table[fd];
      file->offset = 0;
      file->inode_manager = inode_manager;
      file->inode = inode;
      file->ref_count = 1;
      file->read_handle = read_handle;
      file->write_handle = write_handle;
      file->lseek_handle = lseek_handle;
      file->close_handle = close_handle;
      is_free[fd] = 0;
      return fd;
    }
  Panic("File table is full");
  return -1;
}

void file_table_free(file_t *file) {
  Assert(file != NULL);
  file->read_handle = NULL;
  file->write_handle = NULL;
  file->lseek_handle = NULL;
  file->close_handle = NULL;
  file->inode = NULL;
  file->inode_manager = NULL;
  is_free[file - file_table] = 1;
}

file_t *file_table_get(int fd) {
  return &file_table[fd];
}

void file_table_set_permission(int fd, int readable, int writable) {
  file_t *file = &file_table[fd];
  file->readable = readable ? 1 : 0;
  file->writable = writable ? 1 : 0;
}