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

static thread_t *threadlist = NULL;

void threadlist_add(thread_t *thread) {
  thread_t *node = (thread_t *)pmm->alloc(sizeof(thread_t));
  Assert(node != NULL);
  *node = *thread;
  node->next = threadlist;
  threadlist = node;
}

void threadlist_remove(thread_t *thread) {
  thread_t *prev, *cur;

  prev = NULL;
  for (cur = threadlist; cur != NULL; prev = cur, cur = cur->next) {
    if (cur->tid == thread->tid) {
      if (prev == NULL)
        threadlist = cur->next;
      else
        prev->next = cur->next;
      pmm->free(cur);
      return;
    }
  }
  Panic("Should not reach here");
}

void threadlist_print() {
  thread_t *scan;
  for (scan = threadlist; scan != NULL; scan = scan->next) {
    const char *stat = NULL;
    switch (scan->stat) {
      case RUNNING: stat = "RUNNING"; break;
      case RUNNABLE: stat = "RUNNABLE"; break;
      case BLOCKED: stat = "BLOCKED"; break;
      case DEAD: stat = "BLOCKED"; break;
      default: Panic("Should not reach here");
    }
    printf("(tid: %d, stat: %s)", scan->tid, stat);
  }
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
  static int tid = 0;
  _Area stackinfo;

  // tid and stat
  thread->tid = tid++;
  thread->stat = RUNNABLE; 

  // NULL to user
  thread->next = NULL;  

  // allocate stack and prepare regset
  thread->kstack = (uint8_t *)pmm->alloc(MAX_KSTACK_SIZE);
  stackinfo.start = (void *)thread->kstack;
  stackinfo.end = (void*)(thread->kstack + MAX_KSTACK_SIZE);
  thread->regs = _make(stackinfo, (void (*)(void *))entry, arg);
  Log("Create thread (tid: %d), kstack start: %p", 
    thread->tid, stackinfo.start);
  
  // add to threadlist
  threadlist_add(thread);

  return 0;
}

static void kmt_teardown(thread_t *thread) {
  pmm->free(thread->kstack);
  threadlist_remove(thread);
}

static thread_t *kmt_schedule() {
  threadlist_print(); // TO REMOVE
  thread_t *scan;
  for (scan = threadlist; scan != NULL; scan = scan->next) {
    if (scan->stat == RUNNABLE) {
      Log("Schedule to thread (tid %d)", scan->tid);
      return scan;
    }
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