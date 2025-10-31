#include "logger.h"
#include "vga.h"

void log(enum log_type type, const char *str) {
    const char *prefix;
    uint8_t fg_color;

    switch (type) {
        case LOG_INFO:
            prefix = "[INFO]";
            fg_color = VGA_COLOR_CYAN;
            break;
        case LOG_SUCCESS:
            prefix = "[ OK ]";
            fg_color = VGA_COLOR_GREEN;
            break;
        case LOG_WARNING:
            prefix = "[WARN]";
            fg_color = VGA_COLOR_BROWN;
            break;
        case LOG_ERROR:
            prefix = "[FAIL]";
            fg_color = VGA_COLOR_RED;
            break;
        case LOG_DEBUG:
            prefix = "[DBUG]";
            fg_color = VGA_COLOR_LIGHT_GREY;
            break;
        default:
            prefix = "[????]";
            fg_color = VGA_COLOR_WHITE;
            break;
    }

    print_colored(prefix, fg_color, VGA_COLOR_BLACK);
    printf(" %s", str);
}

void log_demo() {
    printf("\n");
    printf("=== Color Log Demo ===\n");
    log_info("[INFO] ");
    printf("This is informational text\n");
    log_success("[SUCCESS] ");
    printf("Operation completed successfully\n");
    log_warning("[WARNING] ");
    printf("This is a warning message\n");
    log_error("[ERROR] ");
    printf("This is an error message\n");
    log_debug("[DEBUG] ");
    printf("Debug information\n");
    printf("\n");
}
