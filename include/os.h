#ifndef __OS_H__
#define __OS_H__

#include <kernel.h>

/*------------------------------------------
                  thread.h
  ------------------------------------------*/

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

thread_t *new_thread(void (*entry)(void *), void *arg);
void delete_thread(thread_t *thread);

/*------------------------------------------
                threadlist.h
  ------------------------------------------*/

extern thread_t *idle;
extern thread_t *cur_thread;

void threadlist_add(thread_t *thread);
thread_t *threadlist_remove(int tid);
void threadlist_print();

/*------------------------------------------
                spinlock.h
  ------------------------------------------*/

struct spinlock {
  int locked;
  const char *name;
};

#define SPINLOCK_INIT(NAME) \
  (struct spinlock) { \
    .locked = 0, \
    .name = (NAME), \
  }

/*------------------------------------------
                threadqueue.h
  ------------------------------------------*/

typedef struct _threadqueue_node {
  struct thread *thread;
  struct _threadqueue_node *next;
} threadqueue_node;

typedef struct _threadqueue {
  threadqueue_node *head;
  threadqueue_node *tail;
  int size;
} threadqueue;

void threadqueue_init(threadqueue *queue);
int threadqueue_empty(threadqueue *queue);
void threadqueue_push(threadqueue *queue, thread_t *thread);
thread_t *threadqueue_pop(threadqueue *queue);

/*------------------------------------------
                semaphore.h
  ------------------------------------------*/

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
  
#endif
