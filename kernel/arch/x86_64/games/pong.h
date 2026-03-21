#ifndef PONG_H
#define PONG_H

#include <stdint.h>
#include <mm.h>

struct obj {
  int x;
  int y;
  int width;
  int height;
  int vel_x;
  int vel_y;
};

/* audio addresses embedded into memory by the linker */
extern virt_addr_t _audio_pong_wall_start;
extern virt_addr_t _audio_pong_wall_end;
extern virt_addr_t _audio_pong_paddle_start;
extern virt_addr_t _audio_pong_paddle_end;
extern virt_addr_t _audio_pong_score_start;
extern virt_addr_t _audio_pong_score_end;

void pong_init(int width, int height);
void pong_start();
void pong_handle_input();
void pong_update(int delta);
void pong_draw();

#endif
