#include <stdint.h>
#include <am.h>
#include <amdev.h>
#include <amdevutil.h>
#include <common.h>

static _Device *input = NULL;
static _Device *timer = NULL;
static _Device *video = NULL;
static _Device *ata0 = NULL;
static _Device *pciconf = NULL;

static _Device *get_device(uint32_t id) {
  _Device *dev;

  for (int n = 1; (dev = _device(n)); n++) 
    if (dev->id == id)
      return dev;
  return NULL;
}

static inline void init_input() {
  input = get_device(_DEV_INPUT);
  Assert(input != NULL);
}

static inline void init_timer() {
  timer = get_device(_DEV_TIMER);
  Assert(timer != NULL);
}

static inline void init_video() {
  video = get_device(_DEV_VIDEO);
  Assert(video != NULL);
}

static inline void init_ata0() {
  ata0 = get_device(_DEV_ATA0);
  Assert(ata0 != NULL);
}

static inline void init_pciconf() {
  pciconf = get_device(_DEV_PCICONF);
  Assert(pciconf != NULL);
}

static inline uint8_t readb(_Device *dev, uint32_t reg) {
  uint8_t res;
  dev->read(reg, &res, 1);
  return res;
}

static inline uint32_t readl(_Device *dev, uint32_t reg) {
  uint32_t res;
  dev->read(reg, &res, 4);
  return res;
}

static inline void writeb(_Device *dev, uint32_t reg, uint8_t res) {
  dev->write(reg, &res, 1);
}

uint32_t uptime() {
  if (!timer) 
    init_timer();

  _UptimeReg uptime;
  timer->read(_DEVREG_TIMER_UPTIME, &uptime, sizeof(uptime));
  return uptime.lo;
}

void read_key(int *key, int *down) {
  if (!input)
    init_input();

  _KbdReg kbd;
  input->read(_DEVREG_INPUT_KBD, &kbd, sizeof(kbd));
  if (key)
    *key = kbd.keycode;
  if (down)
    *down = kbd.keydown;
}

void draw_rect(uint32_t *pixels, int x, int y, int w, int h) {
  if (!video) 
    init_video();
  
  _FBCtlReg ctl;
  ctl.x = x;
  ctl.y = y;
  ctl.w = w;
  ctl.h = h;
  ctl.sync = 1;
  ctl.pixels = pixels;
  video->write(_DEVREG_VIDEO_FBCTL, &ctl, sizeof(ctl));
}

void draw_sync() {
  Assert(0);
}

int screen_width() {
  if (!video) 
    init_video();

  _VideoInfoReg info;
  video->read(_DEVREG_VIDEO_INFO, &info, sizeof(info));
  return info.width;
}

int screen_height() {
  if (!video) 
    init_video();

  _VideoInfoReg info;
  video->read(_DEVREG_VIDEO_INFO, &info, sizeof(info));
  return info.height;
}

uint32_t read_pciconf(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
  if (!pciconf)
    init_pciconf();

  return readl(pciconf, _DEVREG_PCICONF(bus, slot, func, offset));
}

void read_disk(void *dst, int offset) {
  if (!ata0)
    init_ata0();

  while ((readb(ata0, _DEVREG_ATA_STATUS) & 0xc0) != 0x40)
    continue;
  writeb(ata0, _DEVREG_ATA_NSECT,  1);
  writeb(ata0, _DEVREG_ATA_SECT,   offset);
  writeb(ata0, _DEVREG_ATA_CYLOW,  offset >> 8);
  writeb(ata0, _DEVREG_ATA_CYHIGH, offset >> 16);
  writeb(ata0, _DEVREG_ATA_DRIVE,  (offset >> 24) | 0xe0);
  writeb(ata0, _DEVREG_ATA_STATUS, 0x20);
  while ((readb(ata0, _DEVREG_ATA_STATUS) & 0xc0) != 0x40)
    continue;
  for (int i = 0; i < SECTSZ / sizeof(uint32_t); i++) 
    ((uint32_t *)dst)[i] = readl(ata0, _DEVREG_ATA_DATA);
}

