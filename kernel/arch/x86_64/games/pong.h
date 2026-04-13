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

// pixel font to render score
static const uint8_t display[10][7][5] = {
    // 0
    {
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,1,1},
        {1,0,1,0,1},
        {1,1,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    // 1
    [1] = {
        {0,0,1,0,0},
        {0,1,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,1,1,1,0},
    },
    // 2
    [2] = {
        {0,1,1,1,0},
        {1,0,0,0,1},
        {0,0,0,0,1},
        {0,0,0,1,0},
        {0,0,1,0,0},
        {0,1,0,0,0},
        {1,1,1,1,1},
    },
    // 3
    [3] = {
        {0,1,1,1,0},
        {1,0,0,0,1},
        {0,0,0,0,1},
        {0,0,1,1,0},
        {0,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    // 4
    [4] = {
        {0,0,0,1,0},
        {0,0,1,1,0},
        {0,1,0,1,0},
        {1,0,0,1,0},
        {1,1,1,1,1},
        {0,0,0,1,0},
        {0,0,0,1,0},
    },
    // 5
    [5] = {
        {1,1,1,1,1},
        {1,0,0,0,0},
        {1,1,1,1,0},
        {0,0,0,0,1},
        {0,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    // 6
    [6] = {
        {0,1,1,1,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    // 7
    [7] = {
        {1,1,1,1,1},
        {0,0,0,0,1},
        {0,0,0,1,0},
        {0,0,1,0,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
    },
    // 8
    [8] = {
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    // 9
    [9] = {
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,1},
        {0,0,0,0,1},
        {0,0,0,0,1},
        {0,1,1,1,0},
    },
};
#endif
