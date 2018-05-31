#ifndef __OS_H__
#define __OS_H__

#include <kernel.h>
#include <klib.h>

/*------------------------------------------
                  thread.h
  ------------------------------------------*/

#define PGSIZE            4096
#define MAX_KSTACK_SIZE   4 * PGSIZE 
#define MAX_TIMESLICE     2

enum { RUNNABLE, RUNNING, BLOCKED, DEAD };

struct thread {
  int tid;
  int stat;
  int timeslice;
  uint8_t *kstack;
  _RegSet *regs;
  struct thread *next;
};

thread_t *new_thread(void (*entry)(void *), void *arg);
void delete_thread(thread_t *thread);

/*------------------------------------------
                threadlist.h
  ------------------------------------------*/

extern thread_t *idle;
extern thread_t *cur_thread;

void threadlist_add(thread_t *thread);
thread_t *threadlist_remove(int tid);
void threadlist_print();

/*------------------------------------------
                spinlock.h
  ------------------------------------------*/

struct spinlock {
  int locked;
  const char *name;
};

#define SPINLOCK_INIT(NAME) \
  (struct spinlock) { \
    .locked = 0, \
    .name = (NAME), \
  }

/*------------------------------------------
                threadqueue.h
  ------------------------------------------*/

typedef struct _threadqueue_node {
  struct thread *thread;
  struct _threadqueue_node *next;
} threadqueue_node;

typedef struct _threadqueue {
  threadqueue_node *head;
  threadqueue_node *tail;
  int size;
} threadqueue;

void threadqueue_init(threadqueue *queue);
int threadqueue_empty(threadqueue *queue);
void threadqueue_push(threadqueue *queue, thread_t *thread);
thread_t *threadqueue_pop(threadqueue *queue);

/*------------------------------------------
                semaphore.h
  ------------------------------------------*/

struct semaphore {
  int count;
  threadqueue queue;
  struct spinlock lock;
};

#define SEM_INIT(NAME, VALUE) \
  (struct semaphore) { \
    .count = (VALUE), \
    .queue = { NULL, NULL, 0 }, \
    .lock = { 0, (NAME) }, \
  }

/*------------------------------------------
                inode_manager.h
  ------------------------------------------*/

enum {
  INODE_FILE,
  INODE_DIR
};

struct inode {
  char name[MAXPATHLEN];
  int type;
  int mode;
  string_t data;
  struct inode *parent;
  struct inode *child;
  struct inode *next;
  struct inode *prev;
};

// implemented as tree
typedef struct inode_manager {
  inode_t *root;
} inode_manager_t;

void inode_manager_init(inode_manager_t *inode_manager);
void inode_manager_destroy(inode_manager_t *inode_manager);
inode_t *inode_manager_lookup(inode_manager_t *inode_manager, const char *path, 
                              int type, int create, int mode);
void inode_manager_remove(inode_t *inode);
void inode_manager_print(inode_manager_t *inode_manager);

/*------------------------------------------
                  filesystem.h
  ------------------------------------------*/

struct filesystem {
  const char *name;
  inode_manager_t inode_manager;

  // void (*init)(filesystem_t *fs, const char *name, inode_t *dev);
  inode_t *(*lookup)(filesystem_t *fs, const char *path, int flags);
  // int (*close)(inode_t *inode);
};

filesystem_t *new_kvfs(const char *name);
filesystem_t *new_procfs(const char *name);
filesystem_t *new_devfs(const char *name);

/*------------------------------------------
                  fs_manager.h
  ------------------------------------------*/

void fs_manager_init();
int fs_manager_add(const char *path, filesystem_t *fs);
filesystem_t *fs_manager_get(const char *path, char *subpath);
int fs_manager_remove(const char *path);
void fs_manager_print();

/*------------------------------------------
                    file.h
  ------------------------------------------*/

struct file {
  off_t offset;
  inode_t *inode;
  int (*open)(inode_t *inode, file_t *file, int flags);
  ssize_t (*read)(inode_t *inode, file_t *file, char *buf, size_t size);
  ssize_t (*write)(inode_t *inode, file_t *file, const char *buf, size_t size);
  off_t (*lseek)(inode_t *inode, file_t *file, off_t offset, int whence);
};

int new_kvfs_file();
int new_procfs_file();
int new_devfs_file();

void delete_kvfs_file(int fd);
void delete_procfs_file(int fd);
void delete_devfs_file(int fd);

#endif
