#include <kernel/panic.h>
#include <drivers/serial.h>
#include <stdint.h>

struct regs {
	uint64_t rax, rbx, rcx, rdx;
	uint64_t rsi, rdi, rbp, rsp;
	uint64_t r8, r9, r10, r11;
	uint64_t r12, r13, r14, r15;
	uint64_t rflags, cr2, cr3;
};

// GCC won't allow doing all of these at once, have to split
// it up into regions
static void snapshot_regs(struct regs *r) {
	__asm__ volatile (
		"movq %%rax, %0\n"
		"movq %%rbx, %1\n"
		"movq %%rcx, %2\n"
		"movq %%rdx, %3\n"
		: "=m"(r->rax), "=m"(r->rbx), "=m"(r->rcx), "=m"(r->rdx)
		:: "memory"
	);
	__asm__ volatile (
		"movq %%rsi, %0\n"
		"movq %%rdi, %1\n"
		"movq %%rbp, %2\n"
		"movq %%rsp, %3\n"
		: "=m"(r->rsi), "=m"(r->rdi), "=m"(r->rbp), "=m"(r->rsp)
		:: "memory"
	);
	__asm__ volatile (
		"movq %%r8,  %0\n"
		"movq %%r9,  %1\n"
		"movq %%r10, %2\n"
		"movq %%r11, %3\n"
		: "=m"(r->r8), "=m"(r->r9), "=m"(r->r10), "=m"(r->r11)
		:: "memory"
	);
	__asm__ volatile (
		"movq %%r12, %0\n"
		"movq %%r13, %1\n"
		"movq %%r14, %2\n"
		"movq %%r15, %3\n"
		: "=m"(r->r12), "=m"(r->r13), "=m"(r->r14), "=m"(r->r15)
		:: "memory"
	);

	uint64_t rflags;
	__asm__ volatile (
		"pushfq\n"
		"popq %0\n"
		: "=r"(rflags)
	);
	r->rflags = rflags;

	uint64_t cr2, cr3;
	__asm__ volatile ("movq %%cr2, %0" : "=r"(cr2));
	__asm__ volatile ("movq %%cr3, %0" : "=r"(cr3));
	r->cr2 = cr2;
	r->cr3 = cr3;
}

static void serial_hex64(uint64_t val) {
	const char *hex = "0123456789ABCDEF";
	serial_write("0x");
	for (int i = 60; i >= 0; i -= 4)
		serial_putchar(hex[(val >> i) & 0xF]);
}

void _panic(const char *msg, const char *file, int line, const char *func) {
	__asm__ volatile ("cli");

	struct regs r;
	snapshot_regs(&r);

	serial_write("\n--- KERNEL PANIC ---\n");
	serial_write(msg);
	serial_write("\n");
	serial_write(file);
	serial_write(" in ");
	serial_write(func);
	serial_write("()\n");
	serial_write("RSP="); serial_hex64(r.rsp);
	serial_write(" CR2="); serial_hex64(r.cr2);
	serial_write(" CR3="); serial_hex64(r.cr3);
	serial_write("\n");

	// halt
	while (1)
		__asm__ volatile ("hlt");
}
