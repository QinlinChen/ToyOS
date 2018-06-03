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
  s->capacity = 2;
  s->size = 0;
  kmt->spin_init(&s->lock, "string_lock");
}

int string_empty(string_t *s) {
  Assert(s != NULL);
  kmt->spin_lock(&s->lock);
  int is_empty = (s->size == 0);
  kmt->spin_unlock(&s->lock);
  return is_empty;
}

size_t string_length(string_t *s) {
  Assert(s != NULL);
  kmt->spin_lock(&s->lock);
  size_t length = s->size;
  kmt->spin_unlock(&s->lock);
  return length;
}

size_t string_capacity(string_t *s) {
  Assert(s != NULL);
  kmt->spin_lock(&s->lock);
  size_t capacity = s->capacity;
  kmt->spin_unlock(&s->lock);
  return capacity;
}

void string_cat(string_t *s1, const char *s2) {
  Assert(s1 != NULL && s2 != NULL);
  kmt->spin_lock(&s1->lock);
  while (*s2)
    string_push_back(s1, *s2++);
  kmt->spin_unlock(&s1->lock);
}

void string_destroy(string_t *s) {
  Assert(s != NULL);
  kmt->spin_lock(&s->lock);
  pmm->free(s->data);
  s->size = 0;
  s->capacity = 0;
  kmt->spin_unlock(&s->lock);
}

void string_print(string_t *s) {
  Assert(s != NULL);
  kmt->spin_lock(&s->lock);
  for (size_t i = 0; i < s->size; ++i)
    _putc(s->data[i]);
  kmt->spin_unlock(&s->lock);
}

ssize_t string_read(string_t *s, off_t offset, void *buf, size_t size) {
  size_t nleft = size;
  ssize_t nread = 0;
  char *bufp = buf;

  kmt->spin_lock(&s->lock);
  while (nleft > 0 && (size_t)offset < s->size) {
    *bufp++ = s->data[offset++];
    nleft--;
    nread++;
  }
  kmt->spin_unlock(&s->lock);

  return nread;
}

ssize_t string_write(string_t *s, off_t offset, const void *buf, size_t size) {
  size_t nleft = size;
  ssize_t nwritten = 0;
  const char *bufp = buf;

  kmt->spin_lock(&s->lock);
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
  kmt->spin_unlock(&s->lock);
  
  return nwritten; 
}

int string_equal(string_t *s1, const char *s2) {
  Assert(s1 != NULL && s2 != NULL);
  const char *data = s1->data;
  size_t i;
  for (i = 0; i < s1->size; ++i) {
    if (!s2[i] || data[i] != s2[i]) {
      return 0;
    }
  }
  if (s2[i] == '\0')
    return 1;
  return 0;
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

  while((*dst++ = *src++) != '\0')
    continue;

  return ret;
}

char *strcat(char *dst, const char *src) {
  char *ret = dst;

  while (*dst)
    dst++;

  while((*dst++ = *src++) != '\0')
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

void itoa(int x, int base, int sgn, char *dst) {
  static char digits[] = "0123456789abcdef";
  char buf[32];

  int neg = 0;
  uint32_t ux = x;
  if(sgn && x < 0){
    neg = 1;
    ux = -x;
  }

  int i = 0;
  do {
    buf[i++] = digits[ux % base];
  } while((ux /= base) != 0);

  if(neg)
    buf[i++] = '-';
  
  while(--i >= 0)
    *dst++ = buf[i];
  
  *dst = '\0';
}