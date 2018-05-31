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

void *memset(void *s, int c, size_t n) {
  size_t i;

  for (i = 0; i < n; ++i)
    ((uint8_t *)s)[i] = (uint8_t)c;

  return s;
}

void *memcpy(void *dst, const void *src, size_t n) {
  size_t i;

  for (i = 0; i < n; ++i)
    ((uint8_t *)dst)[i] = ((uint8_t *)src)[i];
        
  return dst;
}

size_t strlen(const char* s) {
  size_t n = 0;

  while (s[n])
    n++;

  return n;
}

char *strcpy(char *dst, const char *src) {
  char *ret = dst;

  while((*dst++ = *src++) != 0)
    continue;

  return ret;
}

int strcmp(const char *s1, const char *s2) {
  while(*s1 && *s1 == *s2)
    s1++, s2++;
  return (uint8_t)*s1 - (uint8_t)*s2;
}

char *strchr(const char *s, int ch) {
  if (s == NULL)
    return NULL;
  
  while (*s) {
    if (*s == (char)ch)
      return (char *)s;
    s++;
  }

  return NULL;
}