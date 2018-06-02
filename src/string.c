#include "os.h"
#include "common.h"

static void string_resize(string_t *s, size_t capacity) {
  Assert(s != NULL);
  Assert(capacity >= s->size);
  char *temp = pmm->alloc(capacity);
  for (size_t i = 0; i < s->size; ++i)
    temp[i] = s->data[i];
  pmm->free(s->data);
  s->data = temp;
  s->capacity = capacity;
}

static void string_push_back(string_t *s, char ch) {
  Assert(s != NULL);
  if (s->size == s->capacity)
    string_resize(s, 2 * s->capacity);
  s->data[s->size++] = ch;
}

void string_init(string_t *s) {
  Assert(s != NULL);
  s->data = pmm->alloc(2);
  Assert(s->data != NULL);
  s->size = 0;
  s->capacity = 2;
}

int string_empty(string_t *s) {
  Assert(s != NULL);
  return s->size == 0;
}

size_t string_length(string_t *s) {
  Assert(s != NULL);
  return s->size;
}

size_t string_capacity(string_t *s) {
  Assert(s != NULL);
  return s->capacity;
}

void string_cat(string_t *s1, const char *s2) {
  Assert(s1 != NULL && s2 != NULL);
  while (*s2)
    string_push_back(s1, *s2++);
}

void string_destroy(string_t *s) {
  Assert(s != NULL);
  pmm->free(s->data);
  s->size = 0;
  s->capacity = 0;
}

void string_print(string_t *s) {
  Assert(s != NULL);
  for (size_t i = 0; i < s->size; ++i)
    _putc(s->data[i]);
}

ssize_t string_read(string_t *s, off_t offset, void *buf, size_t size) {
  size_t nleft = size;
  ssize_t nread = 0;
  char *bufp = buf;

  while (nleft > 0 && (size_t)offset < s->size) {
    *bufp++ = s->data[offset++];
    nleft--;
    nread++;
  }

  return nread;
}

ssize_t string_write(string_t *s, off_t offset, const void *buf, size_t size) {
  size_t nleft = size;
  ssize_t nwritten = 0;
  const char *bufp = buf;

  while (nleft > 0 && (size_t)offset < s->size) {
    s->data[offset++] = *bufp++;
    nleft--;
    nwritten++;
  }

  while (nleft > 0) {
    string_push_back(s, *bufp++);
    nleft--;
    nwritten++;
  }
  
  return nwritten; 
}

void *memset(void *s, int c, size_t n) {
  Assert(s != NULL);
  size_t i;

  for (i = 0; i < n; ++i)
    ((uint8_t *)s)[i] = (uint8_t)c;

  return s;
}

void *memcpy(void *dst, const void *src, size_t n) {
  Assert(dst != NULL && src != NULL);
  size_t i;

  for (i = 0; i < n; ++i)
    ((uint8_t *)dst)[i] = ((uint8_t *)src)[i];
        
  return dst;
}

size_t strlen(const char* s) {
  Assert(s != NULL);
  size_t n = 0;

  while (s[n])
    n++;

  return n;
}

char *strcpy(char *dst, const char *src) {
  Assert(dst != NULL && src != NULL);
  char *ret = dst;

  while((*dst++ = *src++) != 0)
    continue;

  return ret;
}

int strcmp(const char *s1, const char *s2) {
  Assert(s1 != NULL && s2 != NULL);
  while(*s1 && *s1 == *s2)
    s1++, s2++;
  return (uint8_t)*s1 - (uint8_t)*s2;
}

char *strchr(const char *s, int ch) {
  Assert(s != NULL);
  if (s == NULL)
    return NULL;
  
  while (*s) {
    if (*s == (char)ch)
      return (char *)s;
    s++;
  }

  return NULL;
}