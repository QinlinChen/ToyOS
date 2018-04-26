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


static void kmt_init() {
  
}

void log_regset(struct _RegSet *r) {
    Log("RegSet: 0x%p\n", r);
    uint32_t *p = (uint32_t *)r;
    for (int i = 0; i < 16; ++i) {
        Log("%x ", p[i]);
        if ((i % 4) == 3)
            Log("\n");
    }
    Log("\n");
}

static int kmt_create(thread_t *thread, void (*entry)(void *arg), void *arg) {
  _Area kstack;
  kstack.start = (void *)thread->kstack;
  kstack.end = (void*)(thread->kstack + MAX_KSTACK_SIZE);
  thread->rs = _make(kstack, (void (*)(void *))entry, arg);
  log_regset(thread->rs);
  return 0;
}

static void kmt_teardown(thread_t *thread) {
  Panic("TODO");
}

static thread_t *kmt_schedule() {
  Panic("TODO");
  return NULL;
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