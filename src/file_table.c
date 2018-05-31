#include "os.h"
#include "common.h"

#define NR_FD 1000

static file_t file_table[NR_FD];
static int is_free[NR_FD];

int file_table_alloc(inode_t *inode, read_handle_t read_handle, write_handle_t write_handle,
                     lseek_handle_t lseek_handle, close_handle_t close_handle) {
  for (int fd = FD_BEGIN; fd < NR_FD; ++fd)
    if (is_free[fd]) {
      file_t *file = &file_table[fd];
      file->offset = 0;
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

void file_table_free(int fd) {
  file_t *file = &file_table[fd];
  file->read_handle = NULL;
  file->write_handle = NULL;
  file->lseek_handle = NULL;
  file->close_handle = NULL;
  is_free[fd] = 1;
}

file_t *file_table_get(int fd) {
  return &file_table[fd];
}