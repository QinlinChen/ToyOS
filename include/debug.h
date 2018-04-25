#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <klib.h>

// assert
#ifdef NDEBUG
  #define assert(ignore) ((void)0)
#else
  #define assert(cond) \
    do { \
      if (!(cond)) { \
        log("asd"); \
        printf("\33[1;31mAssertion fail at %s:%d\33[0m\n", __FILE__, __LINE__); \
        _halt(1); \
      } \
    } while (0)
#endif

// log
#ifdef NLOG
  #define log(format, ...) ((void)0)
#else 
  #define log(format, ...) \
    do { \
      printf("\33[1;34m[%s,%d,%s] " format "\33[0m\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__); \
    } while (0)
#endif

// trace
#ifdef NTRACE
  #define TRACE_ENTRY ((void)0)
  #define TRACE_EXIT ((void)0)
#else
  #define TRACE_ENTRY printf("[trace] %s:entry\n", __func__)
  #define TRACE_EXIT printf("[trace] %s:exit\n", __func__)
#endif

// panic
#define panic(format, ...) \
  do { \
    printf(format, ## __VA_ARGS__); \
    assert(0); \
  } while (0)
  
#endif