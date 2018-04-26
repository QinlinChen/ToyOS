#include <os.h>
#include <common.h>

static void kmt_init();
static int kmt_create(thread_t *thread, void (*entry)(void *arg), void *arg);
static void kmt_teardown(thread_t *thread);
static thread_t *kmt_schedule();
static void kmt_spin_init(spinlock_t *lk, const char *name);
static void kmt_spin_lock(spinlock_t *lk);
static void kmt_spin_unlock(spinlock_t *lk);
static void kmt_sem_init(sem_t *sem, const char *name, int value);
static void kmt_sem_wait(sem_t *sem);
static void kmt_sem_signal(sem_t *sem);

MOD_DEF(kmt) {
  .init = kmt_init,
  .create = kmt_create,
  .teardown = kmt_teardown,
  .schedule = kmt_schedule,
  .spin_init = kmt_spin_init,
  .spin_lock = kmt_spin_lock,
  .spin_unlock = kmt_spin_unlock,
  .sem_init = kmt_sem_init,
  .sem_wait = kmt_sem_wait,
  .sem_signal = kmt_sem_signal,
};

/*------------------------------------------
                thread list
  ------------------------------------------*/

enum { RUNNABLEB, RUNNING, BLOCKED, DEAD };

#define PGSIZE            4096
#define MAX_KSTACK_SIZE   4 * PGSIZE 

typedef struct tcb {
  int tid;
  int stat;
  uint8_t *kstack;
  _RegSet *regs;
  struct tcb *next;
} TCB;

static TCB *threadlist = NULL;

int threadlist_add(void (*entry)(void *arg), void *arg) {
  static int tid = 1;
  _Area stackinfo;

  // allocate node
  TCB *tcb = pmm->alloc(sizeof(TCB));
  Assert(tcb != NULL);

  tcb->tid = tid++;
  tcb->status = RUNNABLE; 

  // allocate stack and prepare regset
  tcb->kstack = (uint8_t *)pmm->alloc(MAX_KSTACK_SIZE);
  stackinfo.start = (void *)thread->kstack;
  stackinfo.end = (void*)(thread->kstack + MAX_KSTACK_SIZE);
  Log("thread (tid: %d), kstack start: %p", tcb->tid, stackinfo.start);
  tcb->regs = _make(stackinfo, (void (*)(void *))entry, arg);

  // insert to list
  tcb->next = threadlist;
  threadlist = tcb;

  return tcb->tid;
}

void threadlist_remove(int tid) {
  TCB *prev, *cur;

  prev = NULL;
  for (cur = threadlist; cur != NULL; prev = cur, cur = cur->next) {
    if (cur->tid == tid) {
      if (prev == NULL)
        threadlist = cur->next;
      else
        prev->next = cur->next;
      pmm->free(cur->kstack);
      pmm->free(cur);
      return;
    }
  }
  Panic("should not reach here");
}

void threadlist_print() {
  TCB *scan;
  for (scan = threadlist; scan != NULL; scan = scan->next) {
    const char *stat;
    switch (scan->stat) {
      case RUNNING: stat = "RUNNING"; break;
      case RUNNABLE: stat = "RUNNABLE"; break;
      case BLOCKED: stat = "BLOCKED"; break;
      case DEAD: stat = "BLOCKED"; break;
      default:  Panic("Should not reach here");
    }
    printf("(tid: %d, stat: %s)", scan->tid, stat);
  }
  printf("\n");
}

/*------------------------------------------
                    thread
  ------------------------------------------*/

thread_t *current = NULL;
thread_t idle;

static void IDLE(void *arg) {
  while (1)
    _putc('.');
}

static void kmt_init() {
  kmt_create(&idle, IDLE, NULL);
}

static int kmt_create(thread_t *thread, void (*entry)(void *arg), void *arg) {
  thread->tid = threadlist_add(entry, arg);
  return 0;
}

static void kmt_teardown(thread_t *thread) {
  threadlist_remove(thread->tid);
}

static thread_t *kmt_schedule() {
  threadlist_print();
  TCB *scan;
  for (scan = threadlist; scan != NULL; scan = scan->next) {
    if (scan->thread != current)
      return scan->thread;
  }
  Panic("IDLE!");
  return &idle;
}

/*------------------------------------------
              mutex and semaphore
  ------------------------------------------*/

static void kmt_spin_init(spinlock_t *lk, const char *name) {
  Panic("TODO");
}

static void kmt_spin_lock(spinlock_t *lk) {
  Panic("TODO");
}

static void kmt_spin_unlock(spinlock_t *lk) {
  Panic("TODO");
}

static void kmt_sem_init(sem_t *sem, const char *name, int value) {
  Panic("TODO");
}

static void kmt_sem_wait(sem_t *sem) {
  Panic("TODO");
}

static void kmt_sem_signal(sem_t *sem) {
  Panic("TODO");
}