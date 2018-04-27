#include <debug.h>

void fence_check(uint8_t *fence) {
  for (int i = 0; i < FENCESIZE; ++i)
    if (fence[i] != FENCEBYTE) 
      Panic("[ERROR] Stack overflow at %p", &fence[i]);
}

void fence_set(uint8_t *target) {
  memset(target, FENCEBYTE, FENCESIZE);
}