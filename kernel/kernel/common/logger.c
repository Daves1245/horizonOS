#include <kernel/logger.h>
#include <kernel/tty.h>
#include <kernel/vga.h>

static void log_prefix(enum log_type type) {
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
    printf(" ");
}

void log(enum log_type type, const char *str) {
    log_prefix(type);
    printf("%s", str);
}

void logf(enum log_type type, const char *format, ...) {
    log_prefix(type);

    va_list args;
    va_start(args, format);
    printf(format, args);
    va_end(args);
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
