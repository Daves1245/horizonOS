/* ansi-colours.h */

#pragma once

/* EXAMPLE:
 *      RED("this text will be red")
 */

#define ANSI_RESET "\x1b[0m"

#define ANSI_RED(text) ("\x1b[31m" text ANSI_RESET)
#define ANSI_GREEN(text) ("\x1b[32m" text ANSI_RESET)
#define ANSI_YELLOW(text) ("\x1b[33m" text ANSI_RESET)
#define ANSI_BLUE(text) ("\x1b[34m" text ANSI_RESET)
#define ANSI_MAGENTA(text) ("\x1b[35m" text ANSI_RESET)
#define ANSI_CYAN(text) ("\x1b[36m" text ANSI_RESET)
