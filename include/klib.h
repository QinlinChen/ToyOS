#ifndef __LIB_H__
#define __LIB_H__

#include <stdint.h>
#include <stddef.h>

// string.h
typedef struct string {
  char *data;
  size_t capacity;
  size_t size;
} string_t;

void string_init(string_t *s);
void string_destroy(string_t *s);
int string_empty(string_t *s);
size_t string_length(string_t *s);
void string_push_back(string_t *s, char ch);
void string_cat(string_t *s1, const char *s2);
void string_print(string_t *s);

void *memset(void *s, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
size_t strlen(const char* s);
char *strcpy(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);
char *strchr(const char *s, int c);

// rand.h
void srand(unsigned int seed);
int rand();
int random(int left, int right);

// printf.h
int printf(const char* fmt, ...);
int sprintf(char* out, const char* format, ...);

#endif