#include <asm/switch.h>
#include <asm/irqflags.h>
#include <kernel/scheduler/process.h>

// TODO(usermode)
// i386 has the same problem as x86_64 here: a switch that crosses into a
// less privileged ring cannot keep using the kernel stack, so the TSS's
// ss0/esp0 have to be repointed at the incoming process' kernel stack.
// i386 can also do the whole switch in hardware through a task gate, but
// that path is slow and long since dead, so we stay with the software
// swtch() below.
void ctx_switch(struct process *new) {
	// TODO(multicore)

	uint32_t flags = irq_save();

	struct process *old = myproc();

	cpus[0].task = new;

	swtch(&old->context.sp, new->context.sp, new->pml4_addr);

	irq_restore(flags);
}
