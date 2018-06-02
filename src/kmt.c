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

thread_t *new_thread(void (*entry)(void *), void *arg) {
  static int tid = 0;
  
  thread_t *thread = (thread_t *)pmm->alloc(sizeof(thread_t));

  // tid, stat, timeslice, next
  thread->tid = tid++;
  thread->stat = RUNNABLE; 
  thread->timeslice = MAX_TIMESLICE;
  thread->next = NULL;  

  // allocate stack and prepare RegSet
  thread->kstack = (uint8_t *)pmm->alloc(MAX_KSTACK_SIZE);

  _Area stackinfo;
#ifdef DEBUG
  // set fence to protect stack
  // we will check fence in os_interrupt
  stackinfo.end = (void *)(thread->kstack + MAX_KSTACK_SIZE);
  fence_set(thread->kstack);
  thread->kstack += FENCESIZE;
  stackinfo.start = (void *)thread->kstack;
#else
  stackinfo.start = (void *)thread->kstack;
  stackinfo.end = (void*)(thread->kstack + MAX_KSTACK_SIZE);
#endif

  thread->regs = _make(stackinfo, (void (*)(void *))entry, arg);
 
  fd_table_t *fd_table = &thread->fd_table;
  fd_table_init(fd_table);
  file_t *ret;
  ret = fd_table_replace(fd_table, STDIN_FILENO, file_table_alloc_stdin());
  Assert(ret == NULL);
  ret = fd_table_replace(fd_table, STDOUT_FILENO, file_table_alloc_stdout());
  Assert(ret == NULL);
  ret = fd_table_replace(fd_table, STDERR_FILENO, file_table_alloc_stderr());
  Assert(ret == NULL);

  Log("Created thread (tid: %d), kstack start: %p", 
    thread->tid, stackinfo.start);
  return thread;
}

void delete_thread(thread_t *thread) {
  thread->stat = DEAD;
  pmm->free(thread->kstack);
  pmm->free(thread);
}

/*------------------------------------------
                  threadlist
  ------------------------------------------*/

// threadlist is THREAD SAFE
static spinlock_t threadlist_lock = SPINLOCK_INIT("threadlist_lock");
static thread_t *threadlist = NULL;
thread_t *idle = NULL;
thread_t *cur_thread = NULL;

void threadlist_add(thread_t *thread) {
  Assert(thread != NULL);

  kmt->spin_lock(&threadlist_lock);
  if (threadlist == NULL) {
    thread->next = threadlist = thread;
  } else {
    thread->next = threadlist->next;
    threadlist->next = thread;
  }
  kmt->spin_unlock(&threadlist_lock);
}

thread_t *threadlist_remove(int tid) {
  Assert(threadlist != NULL);
  thread_t *prev, *cur;

  kmt->spin_lock(&threadlist_lock);
  prev = threadlist;
  for (cur = prev->next; ; prev = cur, cur = cur->next) {
    if (cur->tid == tid)
      break;
    if (cur == threadlist)
      Panic("No thread in list to remove!");
  }
  prev->next = cur->next;
  kmt->spin_unlock(&threadlist_lock);

  return cur;
}

void threadlist_print() {
  if (threadlist == NULL) {
    printf("Threadlist: (null)");
    return;
  }

  kmt->spin_lock(&threadlist_lock);
  for (thread_t *scan = threadlist->next; ; scan = scan->next) {
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

/*------------------------------------------
               thread manager
  ------------------------------------------*/

static void IDLE(void *arg) {
  while (1) {
    continue;
  }
}

static void kmt_init() {
  // create IDLE thread
  // we will not add idle to threadlist
  idle = new_thread(IDLE, NULL);
}

static int kmt_create(thread_t *thread,
  void (*entry)(void *arg), void *arg) {

  thread_t *new_thr = new_thread(entry, arg);
  
  // add thread to list
  threadlist_add(new_thr);

  // add thread info to procfs
  filesystem_t *procfs = fs_manager_get("/proc", NULL);
  char content[512], number[32];
  itoa(new_thr->tid, 10, 1, number);
  strcpy(content, "Thread ");
  strcat(content, number);
  strcat(content, " say hello to you!");
  procfs_add_procinfo(procfs, new_thr->tid, "hello", content, strlen(content));
  
  // only return tid to user
  memset(thread, 0, sizeof(thread_t));
  thread->tid = new_thr->tid;

  return 0;
}

static void kmt_teardown(thread_t *thread) {
  // make sure that user didn't modify
  // member variables that was previously set ZERO
  Assert(thread->stat == 0);
  Assert(thread->timeslice == 0);
  Assert(thread->regs == NULL);
  Assert(thread->kstack == NULL);
  Assert(thread->next == NULL);

  thread_t *thr = threadlist_remove(thread->tid);
  delete_thread(thr);
}

static thread_t *kmt_schedule() {
  // threadlist_print();
  Assert(cur_thread != NULL);
  thread_t *scan;

  // Case 1: cur_thread is idle thread
  if (cur_thread == idle) {

    if (threadlist == NULL) 
      return idle;

    kmt->spin_lock(&threadlist_lock);
    for (scan = threadlist->next; ; scan = scan->next) {
      Assert(scan != NULL);
      if (scan->stat == RUNNABLE) {
        Log("Next thread (tid %d)", scan->tid);
        kmt->spin_unlock(&threadlist_lock);
        return scan;
      }
      if (scan == threadlist->next) {
        kmt->spin_unlock(&threadlist_lock);
        return idle;
      }
    }
    Panic("Should not reach here!");
    kmt->spin_unlock(&threadlist_lock);
  }
  
  // Case 2: cur_thread is in threadlist

  // cur_thread can continue
  if (cur_thread->stat == RUNNABLE && cur_thread->timeslice > 0)
    return cur_thread;

  // Round-Robin
  kmt->spin_lock(&threadlist_lock);
  for (scan = cur_thread->next; ; scan = scan->next) {
    Assert(scan != NULL);
    if (scan->stat == RUNNABLE) {
      Log("Next thread (tid %d)", scan->tid);
      kmt->spin_unlock(&threadlist_lock);
      return scan;
    }
    // if no thread can run, schedule to idle
    if (scan == cur_thread) {
      kmt->spin_unlock(&threadlist_lock);
      return idle;
    }
  }
  Panic("Should not reach here!");
  return idle;
}

/*------------------------------------------
                  spin lock
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

#ifdef DEBUG_LOCK
  Log("%s is locked", lk->name);
#endif
}

static void kmt_spin_unlock(spinlock_t *lk) {

#ifdef DEBUG_LOCK
  Log("%s is unlocked", lk->name);
#endif

  lk->locked = 0;

  pop_intr();
}

/*------------------------------------------
                threadqueue
  ------------------------------------------*/

void threadqueue_init(threadqueue *queue) {
  queue->head = queue->tail = NULL;
  queue->size = 0;
}

int threadqueue_empty(threadqueue *queue) {
  return queue->size == 0;
}

void threadqueue_push(threadqueue *queue, thread_t *thread) {
  threadqueue_node *new_node = (threadqueue_node *)pmm->alloc(
    sizeof(threadqueue_node));
  Assert(new_node != NULL);

  new_node->thread = thread;
  new_node->next = NULL;
  if (queue->size == 0)
    queue->head = new_node;
  else
    queue->tail->next = new_node;
  queue->tail = new_node;
  queue->size++;
}

thread_t *threadqueue_pop(threadqueue *queue) {
  Assert(queue->size != 0);

  thread_t *ret = queue->head->thread;
  threadqueue_node *save = queue->head;
  queue->head = queue->head->next;
  pmm->free(save);
  queue->size--;
  if (queue->size == 0)
    queue->tail = NULL;

  return ret;
}

/*------------------------------------------
        semaphore (only for user thread)
  ------------------------------------------*/

static void kmt_sem_init(sem_t *sem, const char *name, int value) {
  sem->count = value;
  threadqueue_init(&sem->queue);
  kmt_spin_init(&sem->lock, name);
}

static void kmt_sem_wait(sem_t *sem) {
  kmt_spin_lock(&sem->lock);
  sem->count--;
  if (sem->count < 0) {
    Assert(cur_thread != NULL);
    cur_thread->stat = BLOCKED;
    threadqueue_push(&sem->queue, cur_thread);
    kmt_spin_unlock(&sem->lock);
    _yield();
    kmt_spin_lock(&sem->lock);
  }
  kmt_spin_unlock(&sem->lock);
}

static void kmt_sem_signal(sem_t *sem) {
  kmt_spin_lock(&sem->lock);
  sem->count++;
  if (sem->count <= 0) {
    thread_t *towake = threadqueue_pop(&sem->queue);
    Assert(towake->stat == BLOCKED);
    towake->stat = RUNNABLE;
  }
  kmt_spin_unlock(&sem->lock);
}