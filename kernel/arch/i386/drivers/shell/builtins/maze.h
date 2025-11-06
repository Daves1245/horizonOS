#ifndef MAZE_H
#define MAZE_H

#define WALL 0
#define CORRIDOR 1
#define VISITED 2
#define PATH 3
#define SOURCE 4
#define SINK 5

struct cell {
  int top_wall;
  int bottom_wall;
  int right_wall;
  int left_wall;
};

// we leave one row available for drawing text
// this will be hints for what keys to press to generate a maze, solve it, etc.
struct cell grid[24][80];

void draw();
void generate_maze();
void solve_maze();

#endif
