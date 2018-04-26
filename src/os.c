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

void test();

static void os_run() {
  test();
  _intr_write(1); // enable interrupt
  while (1) ; // should never return
}

extern thread_t *current;

static _RegSet *timer_handle(_RegSet *regs) {
  _putc('*');

  // current is not initialized
  if (current == NULL) {
    current = kmt->schedule();  // schedule IDLE
    return current->regs;
  }

  current->timeslice--;
  current->stat = RUNNABLE;
  thread_t *next = kmt->schedule();

  // next is not current
  if (next != current) {
    current->regs = regs;
    current->timeslice = MAX_TIMESLICE;
    current = next;
  }

  current->stat = RUNNING;
  return current->regs;
}

static _RegSet *os_interrupt(_Event ev, _RegSet *regs) {
  switch (ev.event) {
    case _EVENT_IRQ_TIMER: return timer_handle(regs);
    case _EVENT_IRQ_IODEV: _putc('I'); break;
    case _EVENT_ERROR: _putc('x'); _halt(1);
    default: Panic("Not Implemented");
  }
  return NULL; // this is allowed by AM
}