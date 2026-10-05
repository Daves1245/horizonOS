#include <stdint.h>
#include <arch/x86_64/irqflags.h>

uint64_t read_rflags(void) {
	uint64_t flags;
	asm volatile("pushfq; popq %0" : "=r"(flags));
	return flags;
}

// the 'memory' clobber tells the compiler to flush reads and writes by
// declaring this instruction as possibly reading or writing arbitrary
// memory. this isn't cpu-bound, bound all temporary registers used
// by the *compiler* are flushed, guaranteeing that at this point in the
// generated binary, we have done all necessary writes
// TODO feels like this should always be the case for cli/sti, at least
// in the use case of guarding critical sections. worth noting for custom
// language
uint64_t irq_save(void) {
	uint64_t flags = read_rflags() & FL_IF;

	asm volatile("cli" ::: "memory");
	return flags;
}

void irq_restore(uint64_t flags) {
	if (flags & FL_IF) {
		asm volatile("sti" ::: "memory");
	}
}
