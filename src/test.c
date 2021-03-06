#include <am.h>
#include <amdev.h>
#include <amdevutil.h>
#include <common.h>
#include <os.h>

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
static int factor(int n) {
  if (n <= 1)
    return 1;
  return n * factor(n - 1);
}

static void factor_calc(void *arg) {
  int result = factor((int)arg);
  printf("Result %d\n", result);
  while (1);
}

void stackfence_test() {
  thread_t *t = pmm->alloc(sizeof(thread_t));
  kmt->create(t, factor_calc, (void *)520);
} 

/*------------------------------------------
              inode_manager test
  ------------------------------------------*/

int inode_manager_test() {
  inode_manager_t manager;
  inode_t *result;

  inode_manager_init(&manager);
  inode_manager_lookup(&manager, "/bin", INODE_DIR, 1, DEFAULT_MODE);
  inode_manager_lookup(&manager, "/bin", INODE_FILE, 1, DEFAULT_MODE);
  result = inode_manager_lookup(&manager, "/bin", INODE_FILE, 0, 0);
  Assert(result->type == INODE_FILE);
  result = inode_manager_lookup(&manager, "/bin", INODE_DIR, 0, 0);
  Assert(result->type == INODE_DIR);
         
  inode_manager_lookup(&manager, "/usr/cql/ws/oslab", INODE_FILE, 1, DEFAULT_MODE);
  inode_manager_lookup(&manager, "/usr/cql/ws/minilab", INODE_FILE, 1, DEFAULT_MODE);
  inode_manager_lookup(&manager, "/usr/jyy/nb", INODE_FILE, 1, DEFAULT_MODE);

  result = inode_manager_lookup(&manager, "/lib/libc.so", INODE_FILE, 0, 0);
  Assert(result == NULL);
  inode_manager_lookup(&manager, "/lib/libc.so", INODE_FILE, 1, DEFAULT_MODE | S_IXUSR);
  result = inode_manager_lookup(&manager, "/lib/libc.so", INODE_FILE, 0, 0);
  Assert(strcmp(result->name, "libc.so") == 0);
  result = inode_manager_lookup(&manager, "/lib/libc.so", INODE_FILE, 1, 0);
  Assert(strcmp(result->name, "libc.so") == 0);
  result = inode_manager_lookup(&manager, "/lib/libc.so", INODE_FILE, 1, 213);
  Assert(result->mode == (DEFAULT_MODE | S_IXUSR));
  result = inode_manager_lookup(&manager, "/lib/libc.so", INODE_DIR, 0, 0);
  Assert(result == NULL);
  
  inode_manager_lookup(&manager, "/bin/123", INODE_FILE, 1, DEFAULT_MODE);
  inode_manager_lookup(&manager, "/bin/456", INODE_FILE, 1, DEFAULT_MODE);
  inode_manager_print(&manager);
  result = inode_manager_lookup(&manager, "/bin", INODE_DIR, 0, 0);
  inode_manager_remove(&manager, result);
  inode_manager_print(&manager);
  inode_manager_destroy(&manager);
  return 1;
}

/*------------------------------------------
                string test
  ------------------------------------------*/

int string_test() {
  string_t s;
  string_init(&s);
  string_cat(&s, "H");
  string_cat(&s, "el");
  string_cat(&s, "lo, world\n");
  string_print(&s);
  Assert(string_equal(&s, "Hello, world\n") == 1);
  string_write(&s, 11, "abcdefg\n", 8);
  Assert(string_equal(&s, "Hello, worlabcdefg\n") == 1);
  char buf[10];
  string_read(&s, 3, buf, 9);
  buf[9] = '\0';
  Assert(strcmp(buf, "lo, worla") == 0);
  return 1;
}


/*------------------------------------------
              fs_manager test
  ------------------------------------------*/

int fs_manager_test() {
  filesystem_t procfs, kvfs, devfs;
  filesystem_ops_t dumb_ops = { NULL, NULL };
  filesystem_init(&procfs, "procfs", &dumb_ops);
  filesystem_init(&kvfs, "kvfs", &dumb_ops);
  filesystem_init(&devfs, "devfs", &dumb_ops);

  // sequence matters
  Assert(fs_manager_add("/dev", &devfs) == 0);
  Assert(fs_manager_add("/", &kvfs) == 0);
  Assert(fs_manager_add("/proc/", &procfs) == 0);
  Assert(fs_manager_remove("/dev") == &devfs);

  char subpath[MAXPATHLEN];
  filesystem_t *fs;
  Assert((fs = fs_manager_get("/proc/123/stat", subpath)) != NULL);
  Assert(strcmp(fs->name, "procfs") == 0);
  Assert(strcmp(subpath, "/123/stat") == 0);
  
  Assert((fs = fs_manager_get("/proc/", subpath)) != NULL);
  Assert(strcmp(fs->name, "procfs") == 0);
  Assert(strcmp(subpath, "/") == 0);

  Assert((fs = fs_manager_get("/proc", subpath)) != NULL);
  Assert(strcmp(fs->name, "procfs") == 0);
  Assert(strcmp(subpath, "/") == 0);

  Assert((fs = fs_manager_get("/", subpath)) != NULL);
  Assert(strcmp(fs->name, "kvfs") == 0);
  Assert(strcmp(subpath, "/") == 0);

  Assert((fs = fs_manager_get("/pro", subpath)) != NULL);
  Assert(strcmp(fs->name, "kvfs") == 0);
  Assert(strcmp(subpath, "/pro") == 0);

  Assert(fs_manager_remove("/proc") == &procfs);
  Assert(fs_manager_remove("/") == &kvfs);
  return 1;
}

/*------------------------------------------
                    kvfs test
  ------------------------------------------*/

int kvfs_test() {
  filesystem_t *kvfs = fs_manager_get("/", NULL);
  inode_manager_t *manager = &kvfs->inode_manager;
  inode_manager_lookup(manager, "/usr/cql/oslab", INODE_FILE, 1, DEFAULT_MODE);
  inode_manager_lookup(manager, "/usr/cql/minilab", INODE_FILE, 1, S_IRUSR);
  inode_manager_lookup(manager, "/lib/libc.so", INODE_FILE, 1, S_IXUSR);

  Assert(vfs->access("/", F_OK) == 0);
  Assert(vfs->access("/", R_OK) == 0);
  Assert(vfs->access("/", W_OK) == 0);
  Assert(vfs->access("/usr/cql/oslab", F_OK) == 1);
  Assert(vfs->access("/usr/cql/oslab", W_OK | R_OK) == 1);
  Assert(vfs->access("/usr/cql/minilab", F_OK) == 1);
  Assert(vfs->access("/usr/cql/minilab", W_OK | R_OK) == 0);
  Assert(vfs->access("/usr/cql/minilab", R_OK) == 1);
  Assert(vfs->access("/lib/libc.so", X_OK) == 1);
  Assert(vfs->access("/lib/libc.so", W_OK) == 0);
  Assert(vfs->access("/lib/libc.so", R_OK) == 0);

  Assert(vfs->open("/lib/libc.so", O_RDONLY) == -1);
  Assert(vfs->open("/usr/cql/minilabs", O_RDWR) == -1);
  int fd = vfs->open("/usr/cql/oslab", O_RDWR);
  Assert(fd != -1);
  file_t *file = fd_table_get(&cur_thread->fd_table, fd);
  Assert(file->readable);
  Assert(file->writable);
  Assert(file->ref_count == 1);

  int n = 100;
  double d = 3.1415926;
  Assert(vfs->write(fd, &n, sizeof(n)) == sizeof(n));
  Assert(vfs->write(fd, &d, sizeof(d)) == sizeof(d));
  Assert(file->offset == sizeof(n) + sizeof(d));
  
  int n2;
  double d2;
  Assert(vfs->lseek(fd, 0, SEEK_SET) == 0);
  Assert(vfs->read(fd, &n2, sizeof(n2)) == sizeof(n2));
  Assert(n2 == n);
  Assert(vfs->read(fd, &d2, sizeof(d2)) == sizeof(d2));
  Assert(d2 == d);
  Assert(file->offset == sizeof(n) + sizeof(d));

  char buf[10];
  Assert(vfs->read(fd, buf, 10) == 0);
  Assert(vfs->lseek(fd, -sizeof(d), SEEK_CUR) == sizeof(n));
  Assert(vfs->lseek(fd, 0, SEEK_END) == sizeof(n) + sizeof(d));
  Assert(vfs->lseek(fd, 0, SEEK_SET) == 0);
  Assert(vfs->lseek(fd, -1, SEEK_SET) == -1);
  Assert(vfs->read(fd, buf, 100) == sizeof(n) + sizeof(d));
  Assert(file->offset == sizeof(n) + sizeof(d));
  
  Assert(vfs->close(fd) == 0);
  fd = vfs->open("/usr/cql/minilab", O_RDONLY);
  Assert(fd != -1);
  Assert(vfs->write(fd, &n, sizeof(n) == -1));
  Assert(vfs->close(fd) == 0);
  Assert(vfs->close(fd + 1) == -1);

  return 1;
}

/*------------------------------------------
                    devfs test
  ------------------------------------------*/

int devfs_test() {
  Assert(vfs->access("/dev/sda", F_OK) == 0);
  Assert(vfs->access("/dev/zero", W_OK | R_OK) == 1);
  Assert(vfs->access("/dev/null", W_OK | R_OK) == 1);
  Assert(vfs->access("/dev/random", W_OK | R_OK) == 1);

  int n = 123;
  int fd = vfs->open("/dev/zero", O_RDWR);
  Assert(vfs->write(fd, &n, sizeof(n)) == 0);
  Assert(vfs->read(fd, &n, sizeof(n)) == sizeof(n));
  Assert(vfs->lseek(fd, 0, SEEK_END) == 0);
  Assert(n == 0);
  Assert(vfs->close(fd) == 0);

  n = 456;
  fd = vfs->open("/dev/null", O_RDWR);
  Assert(vfs->write(fd, &n, sizeof(n)) == sizeof(n));
  Assert(vfs->read(fd, &n, sizeof(n)) == 0);
  Assert(vfs->lseek(fd, 111, SEEK_CUR) == 0);
  Assert(n == 456);
  Assert(vfs->close(fd) == 0);

  n = 789;
  fd = vfs->open("/dev/random", O_RDWR);
  Assert(vfs->write(fd, &n, sizeof(n)) == 0);
  Assert(n == 789);
  Assert(vfs->read(fd, &n, sizeof(n)) == sizeof(n));
  Assert(vfs->lseek(fd, 222, SEEK_SET) == 0);
  printf("random n = %d\n", n);
  Assert(vfs->close(fd) == 0);

  return 1;
}

/*------------------------------------------
                  procfs test
  ------------------------------------------*/

static void nothing(void *arg) {
  while (1)
    continue;
}

int procfs_test() {
  thread_t thread;
  kmt->create(&thread, nothing, NULL);

  char path[MAXPATHLEN], name[32], buf[1024];
  strcpy(path, "/proc/");
  itoa(thread.tid, 10, 1, name);
  strcat(path, name);
  strcat(path, "/hello");

  Assert(vfs->open("/proc/vgainfo", O_CREAT) == -1);

  int fd;
  size_t size;
  fd = vfs->open(path, O_RDONLY);
  Assert(fd != -1);
  size = vfs->read(fd, buf, 1024);
  buf[size] = '\0';
  printf("%s\n", buf);
  Assert(vfs->close(fd) == 0);

  fd = vfs->open("/proc/cpuinfo", O_RDONLY);
  Assert(fd != -1);
  size = vfs->read(fd, buf, 1024);
  buf[size] = '\0';
  printf("%s\n", buf);
  Assert(vfs->close(fd) == 0);

  fd = vfs->open("/proc/meminfo", O_RDONLY);
  Assert(fd != -1);
  size = vfs->read(fd, buf, 1024);
  buf[size] = '\0';
  printf("%s\n", buf);
  Assert(vfs->close(fd) == 0);

  return 1;
}

/*------------------------------------------
                test run
  ------------------------------------------*/

void test_run(void *arg) {
  Test(inode_manager_test);
  Test(string_test);
  Test(fs_manager_test);
  Test(kvfs_test);
  Test(devfs_test);
  Test(procfs_test);

  char buf[10];
  size_t nread = vfs->read(STDIN_FILENO, buf, 10);
  vfs->write(STDOUT_FILENO, buf, nread);
  printf("\33[1;32mALL TESTS PASSED\33[0m\n");
  while(1)
    continue;
}
