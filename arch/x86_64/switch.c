#include <asm/switch.h>
#include <asm/irqflags.h>
#include <kernel/scheduler/process.h>

// TODO(usermode)
// when we switch to a higher privilege level, we have
// to separate out stacks becuase contents of a less
// privileged stack cannot be trusted. TSS i think
// is how this implemented for x86_64? i know x86
// supported full hardware context switches with it,
// but it's less used in x86_64.
void ctx_switch(struct process *new) {
	// TODO(multicore)

	uint64_t flags = irq_save();

	struct process *old = myproc();

	cpus[0].task = new;

	swtch(&old->context.sp, new->context.sp, new->pml4_addr);

	irq_restore(flags);
}
