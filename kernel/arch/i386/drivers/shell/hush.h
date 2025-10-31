/*
 * hush.h - Horizon Utility Shell
 *
 * basic kernel-space shell for displaying user
 * info and allowing rudimentary interaction while
 * a proper environment is being developed. see also mesh
 *
 */


#ifndef HUSH_H
#define HUSH_H

#include <stdint.h>

#define MAX_COMMAND_LEN 80
#define MAX_ARGS 10
#define HISTORY_LEN 5
#define PROMPT "... "

struct shell_state {
  char command_buffer[MAX_COMMAND_LEN];
  char history[HISTORY_LEN][MAX_COMMAND_LEN];
  int history_index;
  int command_cursor_position;
  int command_len; // running length of command
  int running; // bool
};

struct shell_command {
  const char *name;
  const char *description;
  void (*handler)(int argc, char **argv);
};

int hush_init();
int hush_run();
int hush_execute_command(const char *command, int argc, const char **argv);

#endif
