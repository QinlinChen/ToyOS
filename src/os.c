#include <os.h>
#include <common.h>

static void os_init();
static void os_run();
static _RegSet *os_interrupt(_Event ev, _RegSet *regs);

MOD_DEF(os) {
  .init = os_init,
  .run = os_run,
  .interrupt = os_interrupt,
};

static void os_init() {
  for (const char *p = "Hello, OS World!\n"; *p; p++) {
    _putc(*p);
  }
}

extern void test_run(void *arg);

static void os_run() {
  thread_t test_thread;
  kmt->create(&test_thread, test_run, NULL);
  _intr_write(1); // enable interrupt
  while (1) ; // should never return
}

static _RegSet *switch_thread(_RegSet *regs) {
  // cur_thread is not initialized
  if (cur_thread == NULL) {
    cur_thread = idle;  // schedule IDLE
    return cur_thread->regs;
  }
  
  // consume timeslice and change cur_thread state
  cur_thread->timeslice--;
  if (cur_thread->stat == RUNNING)
    cur_thread->stat = RUNNABLE;

  // decide next thread
  thread_t *next = kmt->schedule();

  // save regs, switch and run
  cur_thread->regs = regs;
  cur_thread = next;
  if (cur_thread->timeslice == 0)
    cur_thread->timeslice = MAX_TIMESLICE;
  cur_thread->stat = RUNNING;
  return cur_thread->regs;
}

static _RegSet *os_interrupt(_Event ev, _RegSet *regs) {

#ifdef DEBUG
  if (cur_thread)
    fence_check(cur_thread->kstack - FENCESIZE);
#endif

  switch (ev.event) {
    case _EVENT_IRQ_TIMER: 
      Log("TimeInterrupt! cur_thread thread (tid %d)", cur_thread->tid); 
      return switch_thread(regs);
    case _EVENT_YIELD: 
      Log("Yield! cur_thread thread (tid %d)", cur_thread->tid);
      return switch_thread(regs);
    case _EVENT_IRQ_IODEV:
      _putc('I');
      break;
    case _EVENT_ERROR:
      _putc('x'); 
      _halt(1);
    default: 
      Panic("Not Implemented");
  }
  return NULL; // this is allowed by AM
}