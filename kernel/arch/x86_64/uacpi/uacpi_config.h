#ifndef UACPI_CONFIG_H
#define UACPI_CONFIG_H

#include <uacpi/helpers.h>
#include <uacpi/log.h>

/*
 *
 * we use barebones mode which provides only table parsing functionality.
 * only these functions are required:
 * - uacpi_kernel_get_rsdp()
 * - uacpi_kernel_map()
 * - uacpi_kernel_unmap()
 * - uacpi_kernel_log()
 */

/*
 * =======================
 * Context-related options
 * =======================
 */
#ifndef UACPI_DEFAULT_LOG_LEVEL
    #define UACPI_DEFAULT_LOG_LEVEL UACPI_LOG_INFO
#endif

#ifndef UACPI_DEFAULT_LOOP_TIMEOUT_SECONDS
    #define UACPI_DEFAULT_LOOP_TIMEOUT_SECONDS 30
#endif

#ifndef UACPI_DEFAULT_MAX_CALL_STACK_DEPTH
    #define UACPI_DEFAULT_MAX_CALL_STACK_DEPTH 256
#endif

/*
 * =========================
 * Platform-specific options
 * =========================
 */

// must be included to compile in barebones mode
#define UACPI_BAREBONES_MODE

// pre-format strings
#define UACPI_USE_BUILTIN_STRING

/*
 * =============
 * Misc. options
 * =============
 */

/*
 * If UACPI_FORMATTED_LOGGING is not enabled, this is the maximum length of the
 * pre-formatted message that is passed to the logging callback.
 */
#ifndef UACPI_PLAIN_LOG_BUFFER_SIZE
    #define UACPI_PLAIN_LOG_BUFFER_SIZE 128
#endif

/*
 * The size of the table descriptor inline storage. All table descriptors past
 * this length will be stored in a dynamically allocated heap array. The size
 * of one table descriptor is approximately 56 bytes.
 */
#ifndef UACPI_STATIC_TABLE_ARRAY_LEN
    #define UACPI_STATIC_TABLE_ARRAY_LEN 16
#endif

UACPI_BUILD_BUG_ON_WITH_MSG(
    UACPI_DEFAULT_LOG_LEVEL < UACPI_LOG_ERROR ||
    UACPI_DEFAULT_LOG_LEVEL > UACPI_LOG_DEBUG,
    "configured default log level is invalid"
);

UACPI_BUILD_BUG_ON_WITH_MSG(
    UACPI_DEFAULT_LOOP_TIMEOUT_SECONDS < 1,
    "configured default loop timeout is invalid (expecting at least 1 second)"
);

UACPI_BUILD_BUG_ON_WITH_MSG(
    UACPI_DEFAULT_MAX_CALL_STACK_DEPTH < 4,
    "configured default max call stack depth is invalid "
    "(expecting at least 4 frames)"
);

UACPI_BUILD_BUG_ON_WITH_MSG(
    UACPI_PLAIN_LOG_BUFFER_SIZE < 16,
    "configured log buffer size is too small (expecting at least 16 bytes)"
);

UACPI_BUILD_BUG_ON_WITH_MSG(
    UACPI_STATIC_TABLE_ARRAY_LEN < 1,
    "configured static table array length is too small (expecting at least 1)"
);

#endif
