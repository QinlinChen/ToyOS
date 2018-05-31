#include <common.h>

static unsigned int next = 1;

void srand(unsigned int seed) {
  next = seed;
}

// [0, 0x7fff]
int rand() {
  next = next * 1103515245 + 12345;
  return ((unsigned)(next / 65536) % 32768);
}

// [left, right)
// right <= 0x3fffffff
int random(int left, int right) {
  Assert(left < right);
  Assert(right < 0x40000000);
  int r = rand() << 15 | rand();
  return (r % (right - left)) + left;
}
