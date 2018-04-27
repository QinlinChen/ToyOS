#ifndef __OS_H__
#define __OS_H__

#include <kernel.h>

static inline void puts(const char *p) {
  for (; *p; p++) {
    _putc(*p);
  }
}

void threadqueue_init(threadqueue *queue);

#endif
