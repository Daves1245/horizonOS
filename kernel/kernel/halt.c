#include "halt.h"

void halt() {
    // while (1) {} is UB
    while (1) {
        asm volatile("hlt");
    }
}

// completely kill the system. no recovery
void system_halt() {
    asm volatile("cli");
    asm volatile("hlt");
}
