#ifndef __OS_H__
#define __OS_H__

#include <kernel.h>

// Thread 
#define PGSIZE            4096
#define MAX_KSTACK_SIZE   4 * PGSIZE 
#define MAX_TIMESLICE     2

enum { RUNNABLE, RUNNING, BLOCKED, DEAD };

struct thread {
  int tid;
  int stat;
  int timeslice;
  uint8_t *kstack;
  _RegSet *regs;
  struct thread *next;
};

struct spinlock {
  int locked;
  const char *name;
};

#define SPINLOCK_INIT(NAME) \
  (struct spinlock) { \
    .locked = 0, \
    .name = (NAME), \
  }

typedef struct _threadqueue_node {
  struct thread *thread;
  struct _threadqueue_node *next;
} threadqueue_node;

typedef struct _threadqueue {
  threadqueue_node *head;
  threadqueue_node *tail;
  int size;
} threadqueue;

struct semaphore {
  int count;
  threadqueue queue;
  struct spinlock lock;
};

#define SEM_INIT(NAME, VALUE) \
  (struct semaphore) { \
    .count = (VALUE), \
    .queue = { NULL, NULL, 0 }, \
    .lock = { 0, (NAME) }, \
  }
  
static inline void puts(const char *p) {
  for (; *p; p++) {
    _putc(*p);
  }
}

#endif
