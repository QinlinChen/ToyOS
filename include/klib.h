#ifndef __LIB_H__
#define __LIB_H__

#include <stdint.h>
#include <stddef.h>

// string.h
void *memset(void *s, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
size_t strlen(const char* s);
char *strcpy(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);
char *strchr(const char *s, int c);

// stdlib.h
void srand(unsigned int seed);
int rand();
int random(int left, int right);

// stdio.h
int printf(const char* fmt, ...);
int sprintf(char* out, const char* format, ...);

#endif