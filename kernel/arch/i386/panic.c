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

static int vga_put_hex(int pos, uint32_t val) {
	const char *hex = "0123456789ABCDEF";
	vga_put(pos++, '0');
	vga_put(pos++, 'x');
	for (int i = 28; i >= 0; i -= 4)
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

static int vga_put_reg(int pos, const char *name, uint32_t val) {
	pos = vga_puts(pos, name);
	pos = vga_puts(pos, "=");
	pos = vga_put_hex(pos, val);
	return pos;
}

struct regs {
	uint32_t eax, ebx, ecx, edx;
	uint32_t esi, edi, ebp, esp;
	uint32_t eflags, cr2, cr3;
};

static void snapshot_regs(struct regs *r) {
	__asm__ volatile (
		"movl %%eax, %0\n"
		"movl %%ebx, %1\n"
		"movl %%ecx, %2\n"
		"movl %%edx, %3\n"
		"movl %%esi, %4\n"
		"movl %%edi, %5\n"
		"movl %%ebp, %6\n"
		"movl %%esp, %7\n"
		: "=m"(r->eax), "=m"(r->ebx), "=m"(r->ecx), "=m"(r->edx),
		  "=m"(r->esi), "=m"(r->edi), "=m"(r->ebp), "=m"(r->esp)
		:
		: "memory"
	);

	uint32_t eflags;
	__asm__ volatile (
		"pushfl\n"
		"popl %0\n"
		: "=r"(eflags)
	);
	r->eflags = eflags;

	uint32_t cr2, cr3;
	__asm__ volatile ("movl %%cr2, %0" : "=r"(cr2));
	__asm__ volatile ("movl %%cr3, %0" : "=r"(cr3));
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

	// line 4-6: registers, 4 per row
	int row = 4;
	struct { const char *name; uint32_t val; } regs[] = {
		{"EAX", r.eax}, {"EBX", r.ebx}, {"ECX", r.ecx}, {"EDX", r.edx},
		{"ESI", r.esi}, {"EDI", r.edi}, {"EBP", r.ebp}, {"ESP", r.esp},
		{"FLG", r.eflags}, {"CR2", r.cr2}, {"CR3", r.cr3},
	};

	for (int i = 0; i < 11; i++) {
		int col = (i % 4) * 16;
		pos = vga_put_reg(row * VGA_WIDTH + col, regs[i].name, regs[i].val);
		if (i % 4 == 3)
			row++;
	}

	// halt
	while (1)
		__asm__ volatile ("hlt");
}
