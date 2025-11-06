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

#define COMMAND_NAME_LEN 10
#define MAX_COMMAND_LEN 80
#define MAX_ARGS 10
#define ARG_LEN 30
#define HISTORY_LEN 5
#define MAX_COMMANDS 20
#define PROMPT "$ "

enum HUSH_STATE {
  OK,
  ERROR,
  INVALID_COMMAND,
  COMMAND_NOT_FOUND
};

/*
 * there needs to be a difference between the full command buffer
 * and the name of a command. for now, we'll use name and arguments
 */
struct hush_state {
  // currently executing command
  char name[COMMAND_NAME_LEN];
  int argc;
  char args[MAX_ARGS][ARG_LEN];

  // buffers and history
  char command_buffer[MAX_COMMAND_LEN];
  char history[HISTORY_LEN][MAX_COMMAND_LEN];
  int history_index;

  // shell state
  int cursor_position;
  int command_len; // running length of command
  int running; // bool
};

struct hush_command {
  const char *name;
  const char *description;
  void (*handler)(int argc, char **argv);
};

void hush_init();
void hush_run();
void hush_handle_keypress(char key);
struct hush_command const * hush_lookup_registry(const char *command);
int hush_parse_entry();
enum HUSH_STATE hush_execute_command();

#endif
