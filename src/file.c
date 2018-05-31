#include "os.h"
#include "common.h"

#define NR_FD 1000

static file_t file_table[NR_FD];
static int is_free[NR_FD];

int new_file() {
    for (int i = 0; i < NR_FD; ++i)
        if (is_free[i]) {
            file_table[i].offset = 0;
            file_table[i].inode = NULL;
            file_table[i].read = NULL;
            return i;
        }
    Panic("File table is full");
    return -1;
}