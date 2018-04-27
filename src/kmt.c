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

// threadlist is THREAD SAFE
static spinlock_t threadlist_lock = SPINLOCK_INITIALIZER("threadlist_lock");
static thread_t *threadlist = NULL;
thread_t idle;
thread_t *current = NULL;

static void threadlist_add(thread_t *thread);
static void threadlist_remove(thread_t *thread);
static void make_thread(thread_t *thread, 
  void (*entry)(void *arg), void *arg);
void threadlist_print();

static void threadlist_add(thread_t *thread) {
  Assert(threadlist != NULL);

  kmt->spin_lock(&threadlist_lock);
  thread_t *node = (thread_t *)pmm->alloc(sizeof(thread_t));
  Assert(node != NULL);
  *node = *thread;
  node->next = threadlist->next;
  threadlist->next = node;
  kmt->spin_unlock(&threadlist_lock);
}

static void threadlist_remove(thread_t *thread) {
  Assert(threadlist != NULL);
  thread_t *prev, *cur;

  kmt->spin_lock(&threadlist_lock);
  prev = threadlist;
  for (cur = prev->next; ; prev = cur, cur = cur->next) {
    if (cur->tid == thread->tid)
      break;
    if (cur == threadlist)
      Panic("No thread in list to remove!");
  }
  prev->next = cur->next;
  pmm->free(cur);
  kmt->spin_unlock(&threadlist_lock);
}

void threadlist_print() {
  Assert(threadlist != NULL);
  thread_t *scan;

  kmt->spin_lock(&threadlist_lock);
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
  kmt->spin_unlock(&threadlist_lock);
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
  Log("Created thread (tid: %d), kstack start: %p", 
    thread->tid, stackinfo.start);
}

static void IDLE(void *arg) {
  while (1)
    continue;
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
  // threadlist_print();
  Assert(current != NULL);

  // current can continue
  if (current->stat == RUNNABLE && current->timeslice > 0)
    return current;

  // Round-Robin
  thread_t *scan;
  kmt->spin_lock(&threadlist_lock);
  for (scan = current->next; ; scan = scan->next) {
    if (scan->stat == RUNNABLE) {
      Log("Next thread (tid %d)", scan->tid);
      kmt->spin_unlock(&threadlist_lock);
      return scan;
    }
    // if no thread can run, schedule to idle
    if (scan == current) {
      kmt->spin_unlock(&threadlist_lock);
      return &idle;
    }
  }
  Panic("Should not reach here!");
  return &idle;
}

/*------------------------------------------
              mutex and semaphore
  ------------------------------------------*/

static void kmt_spin_init(spinlock_t *lk, const char *name) {
  lk->locked = 0;
  lk->name = name;
}

static int nintr = 0;
static int intr_save;

static void push_intr() {
  // It can be the case that we use a lock when
  // interruption is closed, or lock is nested.
  // Therefore, we use a stack to save the state
  // of interruption and handle nested lock.
  int intr_state = _intr_read();
  _intr_write(0);
  if (nintr == 0)
    intr_save = intr_state;
  nintr++;
}

static void pop_intr() {
  Assert(_intr_read() == 0);
  --nintr;
  Assert(nintr >= 0);
  if (nintr == 0 && intr_save == 1)
    _intr_write(1);
}

static void kmt_spin_lock(spinlock_t *lk) {
  push_intr();

  while (_atomic_xchg(&lk->locked, 1) == 1)
    continue;

  Log("%s is locked", lk->name);
}

static void kmt_spin_unlock(spinlock_t *lk) {
  Log("%s is unlocked", lk->name);
  
  lk->locked = 0;

  pop_intr();
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