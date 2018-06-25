#include <am.h>
#include <amdev.h>
#include <amdevutil.h>
#include <klib.h>

typedef struct Screen {
  int fps, width, height;
  uint32_t next_frame;
} SCREEN;

typedef struct Ball {
  int x, y, vx, vy;
  int r;
} BALL;

static void init_screen(SCREEN *screen, int fps) {
  screen->fps = fps;
  screen->width = screen_width();
  screen->height = screen_height();
  screen->next_frame = 0;
}

static void wait_next_frame(SCREEN *screen) {
  while (uptime() < screen->next_frame)
    continue;
  screen->next_frame += 1000 / screen->fps;
}

static void init_ball(BALL *ball, int x, int y, int r) {
  ball->x = x;
  ball->y = y;
  ball->vx = ball->vy = 0;
  ball->r = r;
}

static inline int square(int x) {
  return x * x;
}

static void paint(SCREEN *screen, BALL *ball) {
  static uint32_t cache[1080][1920];
  static uint32_t bg = 0x00000000;
  static uint32_t fg = 0x006a005f;
  for (int y = 0; y < screen->height; y++)
    for (int x = 0; x < screen->width; x++)
      if (square(x - ball->x) + square(y - ball->y) <= square(ball->r))
        cache[y][x] = fg;
      else
        cache[y][x] = bg;
  for (int y = 0; y < screen->height; y++)
    draw_rect(cache[y], 0, y, screen->width, 1);
}

static void show_usage() {
  printf("Welcome to the BALL GAME\n");
  printf("←,↑,→,↓: change the speed of the ball\n");
  printf("s: stop\n");
  printf("q: quit\n");
}

void ball_game() {
  SCREEN screen;
  BALL ball;
  int key, down;
  int leftbound, rightbound, upperbound, lowerbound;

  init_screen(&screen, 60);
  init_ball(&ball, screen.width / 2, screen.height / 2, 50);

  leftbound = ball.r;
  rightbound = screen.width - 1 - ball.r;
  lowerbound = ball.r;
  upperbound = screen.height - 1 - ball.r;

  show_usage();
  while (1) {
    wait_next_frame(&screen);

    read_key(&key, &down);
    if (down) {
      switch (key) {
        case _KEY_UP:     (ball.vy)--; break;
        case _KEY_DOWN:   (ball.vy)++; break;
        case _KEY_LEFT:   (ball.vx)--; break;
        case _KEY_RIGHT:  (ball.vx)++; break;
        case _KEY_S:      ball.vx = ball.vy = 0; break;
        case _KEY_Q:      return;
        default: break;
      }
    }

    ball.x += ball.vx;
    ball.y += ball.vy;
    if (ball.x < leftbound) {
      ball.x = 2 * leftbound - ball.x;
      ball.vx = -ball.vx;
    }
    if (ball.x > rightbound) {
      ball.x = 2 * rightbound - ball.x;
      ball.vx = -ball.vx;
    }
    if (ball.y < lowerbound) {
      ball.y = 2 * lowerbound - ball.y;
      ball.vy = -ball.vy;
    }
    if (ball.y > upperbound) {
      ball.y = 2 * upperbound - ball.y;
      ball.vy = -ball.vy;
    }

    paint(&screen, &ball);
  }
}