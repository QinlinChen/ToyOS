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


thread_t thr[2];
thread_t *current = NULL;

static void kmt_init() {
  
}

static int kmt_create(thread_t *thread, void (*entry)(void *arg), void *arg) {
  _Area kstack;
  kstack.start = (void *)thread->kstack;
  kstack.end = (void*)(thread->kstack + MAX_KSTACK_SIZE);
  Log("kstack start: %p, end: %p", kstack.start, kstack.end);
  thread->regs = _make(kstack, (void (*)(void *))entry, arg);
  return 0;
}

static void kmt_teardown(thread_t *thread) {
  Panic("TODO");
}

static thread_t *kmt_schedule() {
  return (current == &thr[0]) ? &thr[1] : &thr[0];
}

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