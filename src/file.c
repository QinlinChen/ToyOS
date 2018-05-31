#include "os.h"
#include "common.h"

void file_set_permission(file_t *file, int readable, int writable) {
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