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
                    thread
  ------------------------------------------*/

static thread_t idle;
static thread_t *threadlist = NULL;
thread_t *current = NULL;

static void threadlist_add(thread_t *thread);
static void threadlist_remove(thread_t *thread);
static void make_thread(thread_t *thread, 
  void (*entry)(void *arg), void *arg);
void threadlist_print();

static void threadlist_add(thread_t *thread) {
  Assert(threadlist != NULL);
  // TODO: lock()
  thread_t *node = (thread_t *)pmm->alloc(sizeof(thread_t));
  Assert(node != NULL);
  *node = *thread;
  node->next = threadlist->next;
  threadlist->next = node;
  // TODO: unlock()
}

static void threadlist_remove(thread_t *thread) {
  Assert(threadlist != NULL);
  // TODO: lock()
  thread_t *prev, *cur;
  
  prev = threadlist;
  for (cur = prev->next; ; prev = cur, cur = cur->next) {
    if (cur->tid == thread->tid) {
      prev->next = cur->next;
      pmm->free(cur);
      return;
    }
  }
  Panic("Should not reach here");
  // TODO: unlock()
}

void threadlist_print() {
  // TODO: unlock()
  thread_t *scan;
  for (scan = threadlist->next; ; scan = scan->next) {
    const char *stat = NULL;
    switch (scan->stat) {
      case RUNNING: stat = "RUNNING"; break;
      case RUNNABLE: stat = "RUNNABLE"; break;
      case BLOCKED: stat = "BLOCKED"; break;
      case DEAD: stat = "BLOCKED"; break;
      default: Panic("Should not reach here");
    }
    printf("(tid: %d, stat: %s, slice: %d)\n", 
      scan->tid, stat, scan->timeslice);
    if (scan == threadlist)
      break;
  }
  // TODO: unlock()
}

static void make_thread(thread_t *thread, 
  void (*entry)(void *arg), void *arg) {
    
  static int tid = 0;
  _Area stackinfo;

  // tid and stat
  thread->tid = tid++;
  thread->stat = RUNNABLE; 
  thread->timeslice = MAX_TIMESLICE;

  // NULL to user
  thread->next = NULL;  

  // allocate stack and prepare regset
  thread->kstack = (uint8_t *)pmm->alloc(MAX_KSTACK_SIZE);
  stackinfo.start = (void *)thread->kstack;
  stackinfo.end = (void*)(thread->kstack + MAX_KSTACK_SIZE);
  thread->regs = _make(stackinfo, (void (*)(void *))entry, arg);
  Log("Create thread (tid: %d), kstack start: %p", 
    thread->tid, stackinfo.start);
}

static void IDLE(void *arg) {
  while (1)
    _putc('.');
}

static void kmt_init() {
  // make IDLE thread
  make_thread(&idle, IDLE, NULL);
  // avoid to schedule IDLE thread
  idle.stat = BLOCKED;
  // initialize threadlist 
  idle.next = threadlist = &idle;
}

static int kmt_create(thread_t *thread,
  void (*entry)(void *arg), void *arg) {
  make_thread(thread, entry, arg);
  threadlist_add(thread);
  return 0;
}

static void kmt_teardown(thread_t *thread) {
  thread->stat = DEAD;
  threadlist_remove(thread);
  pmm->free(thread->kstack);
}

static thread_t *kmt_schedule() {
  Assert(RR != NULL);
  threadlist_print(); // REMEMBER TO REMOVE

  if (current == NULL) 
    return &idle;
  
  if (current->stat == RUNNABLE && current->timeslice > 0)
    return current;

  thread_t *scan;
  for (scan = current->next; ; scan = scan->next) {
    if (scan->stat == RUNNABLE) {
      Log("Schedule to thread (tid %d)", scan->tid);
      return scan;
    }
    if (scan == current)
      return &idle;
  }
  Panic("Should not reach here!");
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