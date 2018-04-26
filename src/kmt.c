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

typedef struct node {
  thread_t *thread;
  struct node *next;
} node_t;

static node_t *threadlist = NULL;

void threadlist_add(thread_t *thread) {
  node_t *node = pmm->alloc(sizeof(node_t));
  Assert(node != NULL);
  node->thread = thread;
  node->next = threadlist;
  threadlist = node;
}

void threadlist_remove(thread_t *thread) {
  node_t *prev, *cur;

  prev = NULL;
  for (cur = threadlist; cur != NULL; prev = cur, cur = cur->next) {
    if (cur->thread == thread) {
      if (prev == NULL)
        threadlist = cur->next;
      else
        prev->next = cur->next;
      pmm->free(cur);
      return;
    }
  }
  Panic("should not reach here");
}

void threadlist_print() {
  node_t *scan;
  for (scan = threadlist; scan != NULL; scan = scan->next) {
    printf("(tid: %d), ", scan->thread->tid);
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
  static int tid = 1;
  _Area stackinfo;

  thread->tid = tid++;
  thread->status = -1; // TODO
  thread->kstack = (uint8_t *)pmm->alloc(MAX_KSTACK_SIZE);
  stackinfo.start = (void *)thread->kstack;
  stackinfo.end = (void*)(thread->kstack + MAX_KSTACK_SIZE);
  Log("kstack start: %p, end: %p", stackinfo.start, stackinfo.end);
  thread->regs = _make(stackinfo, (void (*)(void *))entry, arg);
  threadlist_add(thread);
  
  return 0;
}

static void kmt_teardown(thread_t *thread) {
  threadlist_remove(thread);
  pmm->free(thread->kstack);
}

static thread_t *kmt_schedule() {
  node_t *scan;
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