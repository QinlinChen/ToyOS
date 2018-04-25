#ifndef __AMDEV_UTIL_H__
#define __AMDEV_UTIL_H__

#include <stdint.h>

// INPUT
void read_key(int *key, int *down);

// TIMER
uint32_t uptime();

// VEDIO
void draw_rect(uint32_t *pixels, int x, int y, int w, int h);
void draw_sync();
int screen_width();
int screen_height();

// PCICONF
uint32_t read_pciconf(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

#define PCI_ID(INFO)      ((uint32_t)(INFO) >> 16)
#define PCI_VENDOR(INFO)  ((INFO) & 0xffff)

// ATA0
#define SECTSZ 512

void read_disk(void *dst, int offset);

#endif