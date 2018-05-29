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

static void os_run() {
  extern void test_run();
  test_run();
  _intr_write(1); // enable interrupt
  while (1) ; // should never return
}

static _RegSet *switch_thread(_RegSet *regs) {
  // current is not initialized
  if (current == NULL) {
    current = idle;  // schedule IDLE
    return current->regs;
  }
  
  // consume timeslice and change current state
  current->timeslice--;
  if (current->stat == RUNNING)
    current->stat = RUNNABLE;

  // decide next thread
  thread_t *next = kmt->schedule();

  // save regs, switch and run
  current->regs = regs;
  current = next;
  if (current->timeslice == 0)
    current->timeslice = MAX_TIMESLICE;
  current->stat = RUNNING;
  return current->regs;
}

static _RegSet *os_interrupt(_Event ev, _RegSet *regs) {

#ifdef DEBUG
  if (current)
    fence_check(current->kstack - FENCESIZE);
#endif

  switch (ev.event) {
    case _EVENT_IRQ_TIMER: 
      Log("TimeInterrupt! current thread (tid %d)", current->tid); 
      return switch_thread(regs);
    case _EVENT_YIELD: 
      Log("Yield! current thread (tid %d)", current->tid);
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