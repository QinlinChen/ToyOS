#include "os.h"
#include "common.h"

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

void file_set_readable(file_t *file, int readable) {
  file->readable = readable ? 1 : 0;
}

void file_set_writable(file_t *file, int writable) {
  file->writable = writable ? 1 : 0;
}

void file_decr_ref_count(file_t *file) {
  file->ref_count--;
}

int file_ref_count_is_zero(file_t *file) {
  return file->ref_count == 0;
}