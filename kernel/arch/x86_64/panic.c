#include <kernel/panic.h>
#include <stdint.h>

#define VGA_BUF ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define PANIC_COLOR 0x4F /* white on red */

static void vga_put(int pos, char c) {
	VGA_BUF[pos] = (uint16_t)c | ((uint16_t)PANIC_COLOR << 8);
}

static int vga_puts(int pos, const char *s) {
	while (*s)
		vga_put(pos++, *s++);
	return pos;
}

static int vga_put_hex(int pos, uint64_t val) {
	const char *hex = "0123456789ABCDEF";
	vga_put(pos++, '0');
	vga_put(pos++, 'x');
	for (int i = 60; i >= 0; i -= 4)
		vga_put(pos++, hex[(val >> i) & 0xF]);
	return pos;
}

static int vga_put_dec(int pos, int val) {
	char buf[12];
	int i = 0;
	if (val < 0) {
		vga_put(pos++, '-');
		val = -val;
	}
	do {
		buf[i++] = '0' + (val % 10);
		val /= 10;
	} while (val);
	while (i--)
		vga_put(pos++, buf[i]);
	return pos;
}

static int vga_put_reg(int pos, const char *name, uint64_t val) {
	pos = vga_puts(pos, name);
	pos = vga_puts(pos, "=");
	pos = vga_put_hex(pos, val);
	return pos;
}

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

void _panic(const char *msg, const char *file, int line, const char *func) {
	__asm__ volatile ("cli");

	struct regs r;
	snapshot_regs(&r);

	// clear screen
	for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
		vga_put(i, ' ');

	int pos;

	// line 0: KERNEL PANIC
	pos = vga_puts(0, "KERNEL PANIC");

	// line 1: message
	pos = vga_puts(VGA_WIDTH, msg);

	// line 2: file:line in func()
	pos = vga_puts(VGA_WIDTH * 2, file);
	pos = vga_puts(pos, ":");
	pos = vga_put_dec(pos, line);
	pos = vga_puts(pos, " in ");
	pos = vga_puts(pos, func);
	pos = vga_puts(pos, "()");

	// line 4-7: registers, 4 per row
	int row = 4;
	struct { const char *name; uint64_t val; } regs[] = {
		{"RAX", r.rax}, {"RBX", r.rbx}, {"RCX", r.rcx}, {"RDX", r.rdx},
		{"RSI", r.rsi}, {"RDI", r.rdi}, {"RBP", r.rbp}, {"RSP", r.rsp},
		{"R8 ", r.r8},  {"R9 ", r.r9},  {"R10", r.r10}, {"R11", r.r11},
		{"R12", r.r12}, {"R13", r.r13}, {"R14", r.r14}, {"R15", r.r15},
		{"FLG", r.rflags}, {"CR2", r.cr2}, {"CR3", r.cr3},
	};

	for (int i = 0; i < 19; i++) {
		int col = (i % 4) * 20;
		pos = vga_put_reg(row * VGA_WIDTH + col, regs[i].name, regs[i].val);
		if (i % 4 == 3)
			row++;
	}

	// halt
	while (1)
		__asm__ volatile ("hlt");
}
