#include <stdarg.h>
#include <common.h>
#include <kernel.h>

/*------------------------------------------
                  string.c
  ------------------------------------------*/

void *memset(void *s, int c, size_t n) {
  size_t i;

  for (i = 0; i < n; ++i)
    ((uint8_t *)s)[i] = (uint8_t)c;

  return s;
}


void *memcpy(void *dst, const void *src, size_t n) {
  int i;

  for (i = 0; i < n; ++i)
    ((uint8_t *)dst)[i] = ((uint8_t *)src)[i];
        
  return dst;
}

size_t strlen(const char* s) {
  int n = 0;

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

/*------------------------------------------
                  stdlib.c
  ------------------------------------------*/

static unsigned int next = 1;

void srand(unsigned int seed) {
  next = seed;
}

// [0, 0x7fff]
int rand() {
  next = next * 1103515245 + 12345;
  return ((unsigned)(next / 65536) % 32768);
}

// [left, right)
// right <= 0x3fffffff
int random(int left, int right) {
  Assert(left < right);
  Assert(right < 0x40000000);
  int r = rand() << 15 | rand();
  return (r % (right - left)) + left;
}

/*------------------------------------------
                  stdio.c
  ------------------------------------------*/

static inline int is_flag(char ch) {
  return strchr("0-+ #", ch) != NULL;
}

static inline int is_digit(char ch) {
  return ('0' <= ch && ch <= '9');
}

static inline int is_length_modifier(char ch) {
  return strchr("hl", ch) != NULL;
}

static inline int is_conversion_specifier(char ch) {
  return strchr("dxXcsp%", ch) != NULL;
}

static int parse_int(const char **pfmt) {
  int num = 0;
  while (is_digit(**pfmt)) {
    num = num * 10 + (**pfmt - '0');
    (*pfmt)++;
  }
  return num;
}

static int print_int(int x, int base, int sgn,
  char flag, int width, int prec, char length) {
  // not enable prec, length
  if (prec || length)
    return -1;
  // enable flag = '0'
  if (flag && flag != '0')
    return -1;

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

  while (i < width) 
    buf[i++] = (flag == '0' ? '0' : ' ');
  
  while(--i >= 0)
    _putc(buf[i]);
 
  return 0;
}

static int print_str(const char *str,
  char flag, int width, int prec, char length) {
  // not enable flag, width, prec, length
  if (flag || width || prec || length)
    return -1;

  if (str == NULL)
    str = "(null)";
  while (*str)
    _putc(*str++);
  return 0;
}

static int print_char(char ch,
  char flag, int width, int prec, char length) {
    // not enable flag, width, prec, length
    if (flag || width || prec || length)
      return -1;

    _putc(ch);
    return 0;
}

spinlock_t lock = SPINLOCK_INIT("print_lock");

int printf(const char *fmt, ...) {
  char flag, length;
  int width, prec, error;
  const char *mark;
  kmt->spin_lock(&lock);
  va_list ap;
  va_start(ap, fmt);

  while (*fmt) {
    if (*fmt != '%') {
      _putc(*fmt++);
    }
    else {
      mark = fmt++;
      flag = length = '\0';
      width = prec = error = 0;

      if (is_flag(*fmt)) 
        flag = *fmt++;
      
      if (is_digit(*fmt)) 
        width = parse_int(&fmt); 
      
      if (*fmt == '.') {
        fmt++;
        prec = parse_int(&fmt);
      }

      // only can parse 'l', 'h'
      if (is_length_modifier(*fmt)) 
        length = *fmt++;

      switch (*fmt) {
        case '%':
          _putc('%'); break;

        case 'd':
          error = print_int(va_arg(ap, int), 10, 1, 
            flag, width, prec, length); break;

        case 'x': case 'X': case 'p':
          error = print_int(va_arg(ap, int), 16, 0,
            flag, width, prec, length); break;

        case 'c': 
          error = print_char((char)va_arg(ap, int),
            flag, width, prec, length); break;

        case 's': 
          error = print_str(va_arg(ap, const char *),
            flag, width, prec, length); break;

        default:  
          error = -1; break; 
      }

      if (*fmt != '\0')
        fmt++;

      // if not implemented or can't parse format
      if (error) 
        while (mark != fmt) 
          _putc(*mark++);
    } 
  }

  va_end(ap);
  kmt->spin_lock(&lock);
  return 0;
}

/*
int sprintf(char* out, const char* format, ...) {

}
*/