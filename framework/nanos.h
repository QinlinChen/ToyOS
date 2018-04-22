#ifndef __NANOS_H__
#define __NANOS_H__

// Constants
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define F_OK 0
#define R_OK 4
#define W_OK 2
#define X_OK 1
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#define MAXPATHLEN 128

// Kernel Panic
#define panic(msg...) \
  do { \
    printf("Kernel panic: " msg); \
    printf("\n"); _halt(1); \
  } while(0)

// Statically-linked Kernel Modules (see defs in kernel.h)
#define MODULE typedef struct
#define MOD_NAME(name) \
  mod_##name##_t; \
  extern mod_##name##_t *name
#define MOD_DEF(name) \
  extern mod_##name##_t __##name##_obj; \
  mod_##name##_t *name = &__##name##_obj; \
  mod_##name##_t __##name##_obj = 

#endif