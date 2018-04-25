#include <os.h>


static void pmm_init();
static void *pmm_alloc(size_t size);
static void pmm_free(void *ptr);


MOD_DEF(pmm) {
  .init = pmm_init,
  .alloc = pmm_alloc,
  .free = pmm_free,
};

static void pmm_init() {

}

static void *pmm_alloc(size_t size) {
  return NULL;
}

static void pmm_free(void *ptr) {
  
}