#include "os.h"
#include "common.h"

static void string_resize(string_t *s, size_t capacity) {
  Assert(capacity >= s->size);
  char *temp = pmm->alloc(capacity);
  for (size_t i = 0; i < s->size; ++i)
    temp[i] = s->data[i];
  pmm->free(s->data);
  s->data = temp;
  s->capacity = capacity;
}

void string_init(string_t *s) {
  s->data = pmm->alloc(2);
  Assert(s->data != NULL);
  s->size = 0;
  s->capacity = 2;
}

int string_empty(string_t *s) {
  return s->size == 0;
}

size_t string_length(string_t *s) {
  return s->size;
}

void string_push_back(string_t *s, char ch) {
  if (s->size == s->capacity)
    string_resize(s, 2 * s->capacity);
  s->data[s->size++] = ch;
}

void string_cat(string_t *s1, const char *s2) {
  while (*s2)
    string_push_back(s1, *s2++);
}

void string_destroy(string_t *s) {
  pmm->free(s->data);
  s->size = 0;
  s->capacity = 0;
}

void string_print(string_t *s) {
  for (size_t i = 0; i < s->size; ++i)
    _putc(s->data[i]);
}