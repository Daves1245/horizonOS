#ifndef PONG_H
#define PONG_H

#include <stdint.h>

struct obj {
  int x;
  int y;
  int width;
  int height;
  int vel_x;
  int vel_y;
};

void pong_init(int width, int height);
void pong_start();
void pong_handle_input();
void pong_update(int delta);
void pong_draw();

#endif
