#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <am.h>
#include <klib.h>

// Log
#ifdef NLOG
  #define Log(format, ...) ((void)0)
#else 
  #define Log(format, ...) \
    do { \
      printf("\33[1;34m[%s,%d,%s] " format "\33[0m\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__); \
    } while (0)
#endif

// Assert
#ifdef NDEBUG
  #define Assert(ignore) ((void)0)
#else
  #define Assert(cond) \
    do { \
      if (!(cond)) { \
        printf("\33[1;31m" "Assertion fail at %s:%d" "\33[0m\n", \
          __FILE__, __LINE__); \
        _halt(1); \
      } \
    } while (0)
#endif

// Trace
#ifdef NTRACE
  #define TRACE_ENTRY ((void)0)
  #define TRACE_EXIT ((void)0)
#else
  #define TRACE_ENTRY \
    printf("\33[1;32m" "[trace] %s: entry" "\33[0m\n" , __func__)
  #define TRACE_EXIT \
    printf("\33[1;32m" "[trace] %s: exit" "\33[0m\n" , __func__)
#endif

// Panic
#define Panic(format, ...) \
  do { \
    printf("\33[1;31m" format "\33[0m\n", ## __VA_ARGS__); \
    Assert(0); \
  } while (0)
  
#endif