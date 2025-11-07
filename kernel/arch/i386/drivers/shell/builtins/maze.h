#ifndef MAZE_H
#define MAZE_H

#include <stdint.h>

#define WALL 0
#define CORRIDOR 1
#define VISITED 2
#define PATH 3
#define SOURCE 4
#define SINK 5

#define MAZE_WIDTH 39
#define MAZE_HEIGHT 10
#define DISPLAY_HEIGHT 23
#define DISPLAY_WIDTH 80

struct cell {
  int type;
  // bools - has wall
  int right_wall;
  int bottom_wall;
};

extern struct cell maze[MAZE_HEIGHT][MAZE_WIDTH];
extern char display_buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];
extern int next_set_id;

void seed_rng(uint32_t seed);
uint32_t random_next();
uint32_t random_range(uint32_t max);

void maze_init();
void draw_maze();
void display_maze();
void generate_maze();
void solve_maze();
void draw_help_text();
void clear_display_buffer();

#endif
