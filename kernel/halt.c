#include "halt.h"

void halt(void) {
	// while (1) {} is UB
	while (1) {
		asm volatile("hlt");
	}
}

// completely kill the system. no recovery
void system_halt(void) {
	asm volatile("cli");
	asm volatile("hlt");
}

// halt and catch fire
void hcf(void) {
	asm volatile("cli");
	while (1) {
		asm volatile("hlt");
	}
}
