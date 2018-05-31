#include "os.h"
#include "common.h"

void file_set_permission(int fd, int readable, int writable) {
  file_t *file = file_table_get(fd);
  file->readable = readable ? 1 : 0;
  file->writable = writable ? 1 : 0;
}