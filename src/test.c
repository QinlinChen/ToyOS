#include <am.h>
#include <amdev.h>
#include <amdevutil.h>
#include <common.h>
#include <kernel.h>

/*------------------------------------------
                  device test
  ------------------------------------------*/

void input_test() {
  printf("Enter q to end input test\n");
  while (1) {
    int key, down;
    read_key(&key, &down);
    if (key == _KEY_Q)
      break;
    if (key != _KEY_NONE) 
      printf("Get key: %d %s\n", key, down ? "down" : "up");
  }
}

void timer_test() {
  uint32_t t0, t1;

  t0 = uptime();
  for (int volatile i = 0; i < 10000000; i ++)
    continue;
  t1 = uptime();

  printf("Loop 10^7 time elapse: %d ms\n", t1 - t0);
}

void video_test() {
  int width = screen_width();
  int height = screen_height();
  uint32_t pixel = 0x006a005f;

  printf("Screen size: %d x %d\n", width, height);
  for (int x = 0; x < 100; x++)
    for (int y = 0; y < 100; y++) 
      draw_rect(&pixel, width / 2 - 50 + x, height / 2 - 50 + y, 1, 1);
  printf("You should see a purple square on the screen.\n");
}

void pciconf_test() {
  for (int bus = 0; bus < 256; bus ++)
    for (int slot = 0; slot < 32; slot ++) {
      uint32_t info = read_pciconf(bus, slot, 0, 0);
      uint16_t id = PCI_ID(info), vendor = PCI_VENDOR(info);
      if (vendor != 0xffff) {
        printf("Get device %d:%d, id %x vendor %x", bus, slot, id, vendor);
        if (id == 0x100e && vendor == 0x8086) 
          printf(" <-- This is an Intel e1000 NIC card!");
        printf("\n");
      }
    }
}

void ata0_test() {
  uint32_t buf[SECTSZ / sizeof(uint32_t)];
  read_disk((void *)buf, 0);

  printf("Reading out the MBR:\n");
  for (int i = 0; i < SECTSZ / 16 / sizeof(uint16_t); i++) {
    for (int j = 0; j < 16; j++) 
      printf("%04x ", ((uint16_t *)buf)[i * 16 + j] & 0xffff);
    printf("\n");
  }
}

void dev_test() {
  _Device *dev;
  for (int n = 1; (dev = _device(n)); n++) {
    printf("* Device: %s\n", dev->name);
    switch (dev->id) {
      case _DEV_INPUT: input_test(); break;
      case _DEV_TIMER: timer_test(); break;
      case _DEV_VIDEO: video_test(); break;
      case _DEV_PCICONF: pciconf_test(); break;
      case _DEV_ATA0: ata0_test(); break;
    }
    printf("\n");
  }
}

/*------------------------------------------
                debug utils test
  ------------------------------------------*/

void debug_test() {
  TRACE_ENTRY;
  Log("This is debug test log");
  Log("This is another debug test log");
  TRACE_EXIT;
  Panic("debug test panic");
}

/*------------------------------------------
                  pmm test
  ------------------------------------------*/

void pmm_test() {
  pmm->free(pmm->alloc(4));
  pmm->free(pmm->alloc(8));
  pmm->free(pmm->alloc(123));
  pmm->free(pmm->alloc(1024));
  pmm->free(pmm->alloc(4096));
}

/*------------------------------------------
                schedule test
  ------------------------------------------*/

static void print_lowercase(void *arg) {
  int volatile count = 0;
  while (1) {
    if (++count == 100000) {
      printf("abcdefg");
      count = 0;
    }
  }
}

static void print_uppercase(void *arg) {
  int volatile count = 0;
  while (1) {
    if (++count == 100000) {
      printf("ABCDEFG");
      count = 0;
    }
  }
}

static void print_number(void *arg) {
  int volatile count = 0;
  while (1) {
    if (++count == 100000) {
      printf("1234567");
      count = 0;
    }
  }
}

// You can see the stack is not destroyed 
// when schedule returns.
void schedule_test() {
  thread_t a, b, c;
  kmt->create(&a, print_lowercase, NULL);
  kmt->create(&b, print_uppercase, NULL);
  kmt->create(&c, print_number, NULL);
}

/*------------------------------------------
                  lock test
  ------------------------------------------*/

static int _sum = 0;
static spinlock_t mutex = SPINLOCK_INIT("sum_lock");

static void addsum(void *arg) {
  kmt->spin_lock(&mutex);
  int N = (int)(intptr_t)arg;
  for (int volatile i = 0; i < N; ++i) {
    _sum++;
  }
  kmt->spin_unlock(&mutex);
  while (1);
}

void lock_test() {
  thread_t a, b, c;
  int N = 10000000;
  kmt->create(&a, addsum, (void *)N);
  kmt->create(&b, addsum, (void *)N);
  kmt->create(&c, addsum, (void *)N);
}

/*------------------------------------------
                  sem test
  ------------------------------------------*/

sem_t empty;
sem_t fill;

static void producer(void *arg) {
  while (1) {
    kmt->sem_wait(&empty);
    printf("(");
    kmt->sem_signal(&fill);
  }
}

static void consumer(void *arg) {
  while (1) {
    kmt->sem_wait(&fill);
    printf(")");
    kmt->sem_signal(&empty);
  }
}

void sem_test(int N) {
  thread_t a, b, c, d;
  kmt->sem_init(&empty, "sem_empty", N);
  kmt->sem_init(&fill, "sem_fill", 0);
  kmt->create(&a, producer, NULL);
  kmt->create(&b, consumer, NULL);
  kmt->create(&c, consumer, NULL);
  kmt->create(&d, consumer, NULL);
}

/*------------------------------------------
      allocate thread concurrently test
  ------------------------------------------*/

static spinlock_t hellolock = SPINLOCK_INIT("hello_lock");

#define MAXN 8
int count = 0;

static void hello(void *arg) {
  int N = (int)arg;
  if (N < MAXN) {
    kmt->spin_lock(&hellolock);
    count++;
    printf("Hello %d\n", count);
    kmt->spin_unlock(&hellolock);
    thread_t *thr1 = pmm->alloc(sizeof(thread_t));
    thread_t *thr2 = pmm->alloc(sizeof(thread_t));
    kmt->create(thr1, hello, (void *)(N + 1));
    kmt->create(thr2, hello, (void *)(N + 1));
  }
  while (1);
}

void hello_test() {
  thread_t t;
  kmt->create(&t, hello, (void *)0);
}

/*------------------------------------------
              stackfence test
  ------------------------------------------*/

// need -O0 to avoid optimization
static int fib(int n) {
  if (n <= 1)
    return 1;
  return fib(n - 1) + fib(n - 2);
}

static void fib_calc(void *arg) {
  int result = fib((int)arg);
  printf("Result %d\n", result);
  while (1);
}

void stackfence_test() {
  thread_t t;
  kmt->create(&t, fib_calc, (void *)500);
} 

void test() {
  stackfence_test();
}
