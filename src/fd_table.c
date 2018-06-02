#include "os.h"
#include "common.h"

void fd_table_init(fd_table_t *fd_table) {
  kmt->init(&fd_table->lock);
  for (int i = 0; i < NR_FD; ++i)
    fd_table->map[i] = NULL;
}

int fd_table_put(fd_table_t *fd_table, file_t *file) {
  kmt->spin_lock(&fd_table->lock);
  for (int i = 0; i < NR_FD; ++i)
    if (fd_table->map[i] == NULL) {
      fd_table->map[i] = file;
      kmt->spin_unlock(&fd_table->lock);
      return i;
    }
  Panic("fd_table is full!");
  return -1;
}

file_t *fd_table_get(fd_table_t *fd_table, int fd) {
  kmt->spin_lock(&fd_table->lock);
  file_t *file = fd_table->map[fd];
  kmt->spin_unlock(&fd_table->lock);
  return file;
}

file_t *fd_table_replace(fd_table_t *fd_table, int fd, file_t *newfile) {
  kmt->spin_lock(&fd_table->lock);
  file_t *oldfile = fd_table->map[fd];
  fd_table->map[fd] = newfile;
  kmt->spin_unlock(&fd_table->lock);
  return oldfile;
}

void fd_table_remove(fd_table_t *fd_table, int fd) {
  kmt->spin_lock(&fd_table->lock);
  file_t *oldfile = fd_table->map[fd];
  fd_table->map[fd] = NULL;
  kmt->spin_unlock(&fd_table->lock);
  return oldfile;
}
