#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <am.h>
#include <nanos.h>

MODULE {
  void (*init)();
  void (*run)();
  _RegSet *(*interrupt)(_Event ev, _RegSet *regs);
} MOD_NAME(os);

MODULE {
  void (*init)();
  void *(*alloc)(size_t size);
  void (*free)(void *ptr);
} MOD_NAME(pmm);

typedef struct thread thread_t;
typedef struct spinlock spinlock_t;
typedef struct semaphore sem_t;
MODULE {
  void (*init)();
  int (*create)(thread_t *thread, void (*entry)(void *arg), void *arg);
  void (*teardown)(thread_t *thread);
  thread_t *(*schedule)();
  void (*spin_init)(spinlock_t *lk, const char *name);
  void (*spin_lock)(spinlock_t *lk);
  void (*spin_unlock)(spinlock_t *lk);
  void (*sem_init)(sem_t *sem, const char *name, int value);
  void (*sem_wait)(sem_t *sem);
  void (*sem_signal)(sem_t *sem);
} MOD_NAME(kmt);

#endif
