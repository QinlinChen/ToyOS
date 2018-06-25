# ToyOS
This is a toy OS constructed during my NJU 2018 spring OS course.
I have just implemented kernel mode.

## Attention
Anyone who attending NJU OS class are **NOT ALLOWED** to copy this code,
or you are responsible for your plagiarism.
However, you are welcome to discuss with me and refer to my code.

## Modules
* `os`: provide interrupt handle

        MODULE {
            void (*init)();
            void (*run)();
            _RegSet *(*interrupt)(_Event ev, _RegSet *regs);
        } MOD_NAME(os);

* `pmm`: phsicial memory management
    
        MODULE {
            void (*init)();
            void *(*alloc)(size_t size);
            void (*free)(void *ptr);
        } MOD_NAME(pmm);

* `kmt`: kernel multi-thread functions

        typedef struct thread thread_t;
        typedef struct spinlock spinlock_t;
        typedef struct semaphore sem_t;
        MODULE {
            void (*init)();
            int (*create)(thread_t *thread, void (*entry)(void *arg), void *arg);
            void (*teardown)(thread_t *thread);
            thread_t *(*schedule)();
            void (*spin_init)(spinlock_t *lk, const char *name);
            void (*spin_lock)(spinlock_t *lk);
            void (*spin_unlock)(spinlock_t *lk);
            void (*sem_init)(sem_t *sem, const char *name, int value);
            void (*sem_wait)(sem_t *sem);
            void (*sem_signal)(sem_t *sem);
        } MOD_NAME(kmt);

* `vfs`: virtual filesystem on RAM

        MODULE {
            void (*init)();
            int (*access)(const char *path, int mode);
            int (*mount)(const char *path, filesystem_t *fs);
            int (*unmount)(const char *path);
            int (*open)(const char *path, int flags);
            ssize_t (*read)(int fd, void *buf, size_t nbyte);
            ssize_t (*write)(int fd, void *buf, size_t nbyte);
            off_t (*lseek)(int fd, off_t offset, int whence);
            int (*close)(int fd);
        } MOD_NAME(vfs);

## Build 
Use `make` to compile the kernel and `make run` to run.

Some macros are defined in debug.h:
* `TEST`: If defined, you will enter test mode and
the kernel will exit after test.

* `GAME`: If defined, you will play a game before kernel's running.

## TODO

* Implement user mode, enable syscall and multiprocess.