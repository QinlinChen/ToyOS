#include <kernel.h>
#include <os.h>
#include <common.h>

int main() {
  // AM initialization
  _ioe_init();
  if (os->interrupt) _asye_init(os->interrupt); 

  // OS module initialization
  if (os->init) os->init();
  if (pmm->init) pmm->init();
  if (vfs->init) vfs->init();
  if (kmt->init) kmt->init();
  
#ifdef TEST
  extern void test_run(void *arg);
  thread_t test_thread;
  kmt->create(&test_thread, test_run, NULL);
#endif

#ifdef GAME
  extern void jmp_game();
  jmp_game();
#endif

  // call os->run()
  if (os->run) os->run();

  _halt(1); // should not reach here
  return -1;
}
