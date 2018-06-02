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
                  string.h
  ------------------------------------------*/

typedef struct string {
  char *data;
  size_t capacity;
  size_t size;
  spinlock_t lock;
} string_t;

// thread safe
void string_init(string_t *s);
void string_destroy(string_t *s);
int string_empty(string_t *s);
size_t string_length(string_t *s);
size_t string_capacity(string_t *s);
void string_cat(string_t *s1, const char *s2);
void string_print(string_t *s);
ssize_t string_read(string_t *s, off_t offset, void *buf, size_t size);
ssize_t string_write(string_t *s, off_t offset, const void *buf, size_t size);

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
  // read, write, lseek and close handles only operate on data.
  string_t data;
};

// implemented as tree
typedef struct inode_manager {
  inode_t *root;
  spinlock_t lock;
} inode_manager_t;

// threadsafe
void inode_manager_init(inode_manager_t *inode_manager);
void inode_manager_destroy(inode_manager_t *inode_manager);
inode_t *inode_manager_lookup(inode_manager_t *inode_manager, const char *path, 
                              int type, int create, int mode);
void inode_manager_remove(inode_manager_t *inode_manager, inode_t *inode);
void inode_manager_print(inode_manager_t *inode_manager);
int inode_manager_checkmode(inode_manager_t *inode_manager, inode_t *inode, int mode);
size_t inode_manager_get_filesize(inode_manager_t *inode_manager, inode_t *inode);
ssize_t inode_manager_read(inode_manager_t *inode_manager, inode_t *inode,
                           off_t offset, void *buf, size_t size);
ssize_t inode_manager_write(inode_manager_t *inode_manager, inode_t *inode,
                            off_t offset, const void *buf, size_t size);

/*------------------------------------------
                    file.h
  ------------------------------------------*/

// Read, write, lseek and close handles only operate on inode.data
// and should be thread safe.
typedef ssize_t (*read_handle_t)(file_t *this, char *buf, size_t size);
typedef ssize_t (*write_handle_t)(file_t *this, const char *buf, size_t size);
typedef off_t (*lseek_handle_t)(file_t *this, off_t offset, int whence);
typedef int (*close_handle_t)(file_t *this);

typedef struct file_ops {
  read_handle_t read_handle;
  write_handle_t write_handle;
  lseek_handle_t lseek_handle;
  close_handle_t close_handle;
} file_ops_t;

struct file {
  off_t offset;
  inode_t *inode;
  inode_manager_t *inode_manager;
  int ref_count;
  int readable;
  int writable;
  // thread safe
  spinlock_t lock;
  file_ops_t ops;
};

/*------------------------------------------
                file_table.h
  ------------------------------------------*/

// thread safe
void file_table_init();
int file_table_alloc(inode_t *inode, inode_manager_t *inode_manager,
                     int readable, int writable, file_ops_t *ops);
void file_table_free(file_t *file);
file_t *file_table_get(int fd);

/*------------------------------------------
                  filesystem.h
  ------------------------------------------*/

// should be thread safe
typedef int (*access_handle_t)(filesystem_t *this, const char *path, int mode);
typedef int (*open_handle_t)(filesystem_t *this, const char *path, int flags);

typedef struct filesystem_ops {
  access_handle_t access_handle;
  open_handle_t open_handle;
} filesystem_ops_t;


struct filesystem {
  const char *name;
  inode_manager_t inode_manager;
  spinlock_t lock;
  filesystem_ops_t ops;
};

// thread safe
void filesystem_init(filesystem_t *fs, const char *name, filesystem_ops_t *ops);
void filesystem_destroy(filesystem_t *fs);
filesystem_t *new_filesystem(const char *name, filesystem_ops_t *ops);
void delete_filesystem(filesystem_t *fs);

// three filesystems' factory function
filesystem_t *new_kvfs(const char *name);
filesystem_t *new_procfs(const char *name);
filesystem_t *new_devfs(const char *name);

/*------------------------------------------
                  fs_manager.h
  ------------------------------------------*/

// thread safe
void fs_manager_init();
int fs_manager_add(const char *path, filesystem_t *fs);
filesystem_t *fs_manager_get(const char *path, char *subpath);
filesystem_t *fs_manager_remove(const char *path);
void fs_manager_print();

#endif
