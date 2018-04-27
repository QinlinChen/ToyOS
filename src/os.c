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
extern thread_t idle;

static _RegSet *timer_handle(_RegSet *regs) {
  _putc('*');

  // current is not initialized
  if (current == NULL) {
    current = &idle;  // schedule IDLE
    return current->regs;
  }
  
  Log("Interrupt eip: %p", regs->eip);
  extern int _sum;
  printf("sum = %d\n", _sum);

  // consume timeslice and change state
  current->timeslice--;
  if (current->stat == RUNNING)
    current->stat = RUNNABLE;

  thread_t *next = kmt->schedule();

  // save regs
  current->regs = regs;

  // if current is yield, we give it bonus TIMESLICE
  // regardless current->timeslice is not 0
  if (next != current) 
    current->timeslice = MAX_TIMESLICE;
  
  // switch and run
  current = next;
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