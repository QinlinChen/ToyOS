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
  // locked by inode_manager
  char name[MAXPATHLEN];
  int type;
  int mode;
  struct inode *parent;
  struct inode *child;
  struct inode *next;
  struct inode *prev;
  // string_t is thread safe.
  // Since read, write, lseek and close only have access to data,
  // it is safe.
  string_t data;
};

// implemented as tree
typedef struct inode_manager {
  inode_t *root;
} inode_manager_t;

void inode_manager_init(inode_manager_t *inode_manager);
void inode_manager_destroy(inode_manager_t *inode_manager);
inode_t *inode_manager_lookup(inode_manager_t *inode_manager, const char *path, 
                              int type, int create, int mode);
void inode_manager_remove(inode_manager_t *inode_manager, inode_t *inode);
void inode_manager_print(inode_manager_t *inode_manager);
int inode_manager_checkmode(inode_manager_t *inode_manager, inode_t *inode, int mode);

/*------------------------------------------
                    file.h
  ------------------------------------------*/

// read, write, lseek and close only have access to inode.data
typedef ssize_t (*read_handle_t)(file_t *this, char *buf, size_t size);
typedef ssize_t (*write_handle_t)(file_t *this, const char *buf, size_t size);
typedef off_t (*lseek_handle_t)(file_t *this, off_t offset, int whence);
typedef int (*close_handle_t)(file_t *this);

struct file {
  off_t offset;
  inode_t *inode;
  int ref_count;
  int readable;
  int writable;
  read_handle_t read_handle;
  write_handle_t write_handle;
  lseek_handle_t lseek_handle;
  close_handle_t close_handle;
};

void file_set_permission(int fd, int readable, int writable);

/*------------------------------------------
                file_table.h
  ------------------------------------------*/

void file_table_init();
int file_table_alloc(inode_t *inode, read_handle_t read_handle, write_handle_t write_handle,
                     lseek_handle_t lseek_handle, close_handle_t close_handle);
void file_table_free(file_t *file);
file_t *file_table_get(int fd);

/*------------------------------------------
                  filesystem.h
  ------------------------------------------*/

typedef int (*access_handle_t)(filesystem_t *this, const char *path, int mode);
typedef int (*open_handle_t)(filesystem_t *this, const char *path, int flags);

struct filesystem {
  const char *name;
  inode_manager_t inode_manager;
  access_handle_t access_handle;
  open_handle_t open_handle;
};

void filesystem_init(filesystem_t *fs, const char *name,
                     access_handle_t access_handle, open_handle_t open_handle);
void filesystem_destroy(filesystem_t *fs);
filesystem_t *new_filesystem(const char *name, access_handle_t access_handle,
                             open_handle_t open_handle);
void delete_filesystem(filesystem_t *fs);

// three filesystems' factory function
filesystem_t *new_kvfs(const char *name);
filesystem_t *new_procfs(const char *name);
filesystem_t *new_devfs(const char *name);

/*------------------------------------------
                  fs_manager.h
  ------------------------------------------*/

void fs_manager_init();
int fs_manager_add(const char *path, filesystem_t *fs);
filesystem_t *fs_manager_get(const char *path, char *subpath);
filesystem_t *fs_manager_remove(const char *path);
void fs_manager_print();

#endif
