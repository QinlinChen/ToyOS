#include <am.h>
#include <amdev.h>
#include <amdevutil.h>
#include <common.h>
#include <kernel.h>

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

void debug_test() {
  TRACE_ENTRY;
  Log("This is debug test log");
  Log("This is another debug test log");
  TRACE_EXIT;
  Panic("debug test panic");
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

void pmm_test() {
  pmm->free(pmm->alloc(4));
  pmm->free(pmm->alloc(8));
  pmm->free(pmm->alloc(123));
  pmm->free(pmm->alloc(1024));
  pmm->free(pmm->alloc(4096));
}

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

void schedule_test() {
  thread_t a, b, c;
  kmt->create(&a, print_lowercase, NULL);
  kmt->create(&b, print_uppercase, NULL);
  kmt->create(&c, print_number, NULL);
}

int _sum = 0;

static void addsum(void *arg) {
  int N = (int)(intptr_t)arg;
  for (int volatile i = 0; i < N; ++i)
    _sum++;

  printf("%d ", _sum);
  while (1);
}

void lock_test() {
  thread_t a, b;
  int N = 10000000;
  kmt->create(&a, addsum, (void *)N);
  kmt->create(&b, addsum, (void *)N);
}

void test() {
  lock_test();
}
