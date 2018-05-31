#include "os.h"
#include "common.h"

void file_set_permission(int fd, int readable, int writable) {
  file_t *file = file_table_get(fd);
  file->readable = readable ? 1 : 0;
  file->writable = writable ? 1 : 0;
}

string_t *file_get_data(file_t *file) {
  Assert(file != NULL);
  return &file->inode->data;
}

void file_set_offset(file_t *file, off_t offset) {
  Assert(file != NULL);
  file->offset = offset;
}

off_t file_get_offset(file_t *file) {
  Assert(file != NULL);
  return file->offset;
}