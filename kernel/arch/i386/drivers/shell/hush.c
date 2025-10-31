#include "hush.h"

struct shell_state hush_state;
struct shell_command current_command;

void hush_init() {

}

void hush_run() {

}

void hush_execute_command() {

}

void parse_command_buffer() {

}

void hush_handle_keypress(char key) {
    if (key == '\n') {
        parse_command_buffer(hush_state.command_buffer);
    }
}
