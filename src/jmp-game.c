#include <am.h>
#include <amdev.h>
#include <amdevutil.h>
#include <klib.h>
#include <common.h>

/* ----------------------------------
 *              screen
 * ---------------------------------- */

struct {
  int fps, width, height;
  uint32_t next_frame;
} screen;

static void init_screen(int fps) {
  screen.fps = fps;
  screen.width = screen_width();
  screen.height = screen_height();
  screen.next_frame = 0;
}

static void wait_next_frame() {
  while (uptime() < screen.next_frame)
    continue;
  screen.next_frame += 1000 / screen.fps;
}

/* ----------------------------------
 *         you and platfomrs
 * ---------------------------------- */

#define MAX_SIZE 4096
#define MAX_POWER 30
#define MAX_DIST 40
#define MAX_RADIUS 10

#define YOU_RADIUS 2
#define PLATFORM_THICKNESS 3

#define OX 100 // origin point

struct {
  int power;
  int y;
} you;

static void init_you() {
  you.power = 0;
  you.y = 0;
}

static void incr_power() {
  if (you.power < MAX_POWER)
    you.power++;
}

struct {
  int head, tail;
  int x[MAX_SIZE];
  int radius[MAX_SIZE];
} platforms;

static void init_platforms() {
  platforms.head = platforms.tail = 0;
}

static void push_back_platform(int x, int radius) {
  platforms.x[platforms.tail] = x;
  platforms.radius[platforms.tail] = radius;
  platforms.tail++;
  Assert(platforms.tail <= MAX_SIZE);
}

static inline int rand_from_range(int begin, int end) {
  Assert(begin < end);
  return rand() % (end - begin) + begin;
}

static void rand_platform() {
  int x = platforms.x[platforms.tail - 1];
  int r = platforms.radius[platforms.tail - 1];
  int nr = rand_from_range(YOU_RADIUS, MAX_RADIUS);
  int nx = x + rand_from_range(r + nr + 1, MAX_DIST);
  push_back_platform(nx, nr);
}

/* ----------------------------------
 *              paint
 * ---------------------------------- */

static uint32_t cache[1080][1920];
static uint32_t black = 0x00000000;
static uint32_t gay_purple = 0x006a005f;
static uint32_t yellow = 0x00ffff00;
static uint32_t red = 0x00ff0000;

static void paint_fill(uint32_t color) {
  for (int y = 0; y < screen.height; y++)
    for (int x = 0; x < screen.width; x++)
        cache[y][x] = color;
}

static void paint_rect(uint32_t color, int x, int y, int w, int h) {
  for (int j = y; j < y + h && j < screen.height; ++j) 
    if (j >= 0)
      for (int i = x; i < x + w && i < screen.width; ++i)
        if (i >= 0)
          cache[j][i] = color;
}

static void paint_you(uint32_t color) {
  int you_height = MAX_POWER - you.power + 5;
  int oy = screen.height / 2;

  paint_rect(color, OX - YOU_RADIUS, oy - you.y - you_height,
             YOU_RADIUS * 2, you_height);
}

static void paint_platforms(uint32_t color) {
  for (int i = platforms.head; i < platforms.tail; ++i)
    paint_rect(color, platforms.x[i] - platforms.radius[i],
               screen.height / 2, platforms.radius[i] * 2, PLATFORM_THICKNESS);
}

static void paint_warning() {
  static int w = 20;
  paint_rect(red, 0, 0, screen.width, w);
  paint_rect(red, 0, screen.height - w, screen.width, w);
  paint_rect(red, 0, 0, w, screen.height);
  paint_rect(red, screen.width - w, 0, w, screen.height);
}

static void repaint() {
  paint_fill(black);
  paint_you(gay_purple);
  paint_platforms(yellow);
}

static void update() {
  for (int y = 0; y < screen.height; y++)
    draw_rect(cache[y], 0, y, screen.width, 1);
}

/* ----------------------------------
 *              main
 * ---------------------------------- */

static int state, fps, score, bonus;

static void init_game() {
  state = 0;
  fps = 60;
  bonus = 1;
  score = 0;
  
  printf("------------------------------\n");
  printf("      score = 0\n");
  init_screen(fps);
  init_you();
  init_platforms();
  push_back_platform(OX, MAX_RADIUS);
  rand_platform();
  repaint();
  update();
}

static void jmp() {
  while (1) {
    wait_next_frame();
    if (you.power <= 0 && you.y <= 0)
      break;
    if (you.power > 0) {
      you.power--;
      you.y += 2;
    } else {
      you.y -= 2;
    }
    for (int i = platforms.head; i < platforms.tail; ++i)
      platforms.x[i]--;
    repaint();
    update();
  }
}

static void show_usage() {
  printf("Welcome to my jump game!\n");
  printf("Press SPACE to jmp\n");
  printf("Enter R to restart and q to quit\n");
  printf("Enjoy it!\n");
}

void jmp_game() {
  show_usage();
  init_game();

  while (1) {
    wait_next_frame();

    int key, down;
    read_key(&key, &down);

    if (key == _KEY_Q)
      break;

    if (key == _KEY_R && down) {
      init_game();
      continue;
    }

    if (key != _KEY_SPACE)
      continue;

    if (state == 0) {
      if (down) {
        incr_power();
        repaint();
        update();
        state = 1;
      }
    }
    else if (state == 1) {
      if (down) {
        incr_power();
        repaint();
        update();
      } else {
        jmp();
        int x = platforms.x[platforms.tail - 1];
        int r = platforms.radius[platforms.tail - 1];
        if (x - r >= OX + YOU_RADIUS || x + r <= OX - YOU_RADIUS) {
          state = 3;
          paint_warning();
          update();
        } else {
          if(x >= OX - 1 && x <= OX + 1) 
            bonus *= 2;
          else
            bonus = 1;
          score += bonus;
          printf("+%d    score = %d\n", bonus, score);
          rand_platform();
          repaint();
          update();
          state = 0;
        }
      }
    }

  }
}