#include <os.h>
#include <common.h>

static char *pmm_brk;
static void pmm_init();
static void *pmm_sbrk(int incr);
static void *pmm_alloc(size_t size);
static void pmm_free(void *ptr);

MOD_DEF(pmm) {
  .init = pmm_init,
  .alloc = pmm_alloc,
  .free = pmm_free,
};

/*------------------------------------------
                  freelist
  ------------------------------------------*/

#define CHUCKSIZE  (64 << 10)    // 64KB

typedef long Align;

union header {
  struct {
    union header *next;
    size_t size;
  };
  Align x;
};

typedef union header Header;

static Header base;
static Header *freelistp; // implemented as circular linked list

static void freelist_init();
static Header *freelist_extend(size_t size);
static void *freelist_alloc(size_t size);
static void freelist_free(void *ap);

static inline size_t size_aligned(size_t size, size_t align) {
  return ((size + align - 1) / align) * align;
}

static inline char *addr_aligned(char *addr, size_t align) {
  return (char *)(((intptr_t)addr + align - 1) & ~(align - 1));
}

static void freelist_init() {
  base.next = freelistp = &base;
  base.size = 0;
}

static Header *freelist_extend(size_t size) {
  void *p;
  Header *hp;

  if ((size = size_aligned(size, sizeof(Header))) < CHUCKSIZE)
    size = CHUCKSIZE;

  if ((p = pmm_sbrk(size)) == (void *)-1)
    return NULL;

  hp = (Header *)p;
  hp->size = size;
  freelist_free((void *)(hp + 1));

  return freelistp;
}

static void freelist_free(void *ap) {
  Header *bp, *pos;

  bp = (Header *)ap - 1;  // block to free

  // search the position to insert block in free list
  for (pos = freelistp; !(bp > pos && bp < pos->next); pos = pos->next)
    if (pos >= pos->next && (bp > pos || bp < pos->next))
      break;  // block is at the end or the beginning of free list

  // try to merge with the previous block
  if ((char *)bp + bp->size == (char *)pos->next) {
    bp->size += pos->next->size;
    bp->next = pos->next->next;
  } else
    bp->next = pos->next;

  // try to merge with the next block
  if ((char *)pos + pos->size == (char *)bp) {
    pos->size += bp->size;
    pos->next = bp->next;
  } else
    pos->next = bp;
    
  // LIFO schema
  freelistp = pos;
}

static void *freelist_alloc(size_t size) {
  Header *cur, *prev;

  if (freelistp == NULL) 
    freelist_init();

  // adjust to align
  size = size_aligned(size, sizeof(Header)) + sizeof(Header);

  prev = freelistp;
  for (cur = prev->next; ; prev = cur, cur = cur->next) {
    // find a block that fits size
    if (cur->size >= size) {
      if (cur->size == size)
        prev->next = cur->next;
      else {
        cur->size -= size;
        cur = (Header *)((char *)cur + cur->size);
        cur->size = size;
      }
      freelistp = prev;
      return (void *)(cur + 1);
    }
    // if can't find, acquire a new area of heap
    if (cur == freelistp)
      if((cur = freelist_extend(size)) == NULL)
        return NULL;
  }
}

static void *addr_aligned_alloc(size_t size) {
  // new size to align
  size_t new_size = 1;
  while (new_size < size)
    new_size <<= 1;
  
  // alloc twice bigger size to ensure
  // enough memory starting from the address to align
  char *addr = (char *)freelist_alloc(new_size * 2);
  char *new_addr = addr_aligned(addr, new_size);
  if (new_addr == addr)
    return (void *)addr;
  
  int gap = new_addr - addr;
  Assert(gap >= sizeof(Header));
  Header *bp = (Header *)addr - 1;
  Header *new_bp = (Header *)new_addr - 1;
  new_bp->size = bp->size - gap;
  bp->size = gap;
  freelist_free(addr);

  return new_addr;
}

static void pmm_init() {
  pmm_brk = addr_aligned((char *)_heap.start, sizeof(Header));
  Log("pmm_brk initialized as %p", pmm_brk);
  Log("_heap = [%08x, %08x)", _heap.start, _heap.end);
}

static void *pmm_sbrk(int incr) {
  char *old_brk = pmm_brk;

  if ((incr < 0) || (pmm_brk + incr > (char *)_heap.end)) {
    printf("ERROR: pmm_sbrk failed. Ran out of memory.\n");
    return (void *)-1;
  }

  pmm_brk += incr;
  return (void *)old_brk;
}

static void *pmm_alloc(size_t size) {
  void *ret = addr_aligned_alloc(size);
  Log("addr: %p, size: %d", ret, size);
  return ret;
}

static void pmm_free(void *ptr) {
  freelist_free(ptr);
  Log("addr: %p", ptr);
}