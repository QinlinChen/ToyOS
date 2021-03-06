#include <stdarg.h>
#include <common.h>
#include <os.h>
#include <amdev.h>
#include <amdevutil.h>

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

int printf(const char *fmt, ...) {
  char flag, length;
  int width, prec, error;
  const char *mark;
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
  return 0;
}

/*
int sprintf(char* out, const char* format, ...) {

}
*/

static const char keycode[] = 
"??????????????"
"`1234567890-=\b"
"\tqwertyuiop[]\\"
"?asdfghjkl;'\n"
"?zxcvbnm,./?"
"??? ??"
"??????"
"????";

char getc() {
  int key, down;
  do {
    read_key(&key, &down);
  } while ((key == _KEY_NONE) || !down);
  return keycode[key];
}