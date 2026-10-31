/*
 * swtch() following i386 calling conventions in C, the only registers worth
 * saving are the callee-saved set plus the stack pointer.
 * everything else is the responsibility of the caller.
 */
#ifndef I386_SWITCH_H
#define I386_SWITCH_H

#include <stdint.h>
#include <asm/mm.h>

struct process;

struct context {
	/* stack frame */
	virt_addr_t sp;

	int intena;
	int noff;

	/* saved by the calle according to the i386 System V ABI.
	 * the four swtch() has to carry across the switch. no red zone, unlike AMD64
	 * */
	uint32_t ebx;
	uint32_t esi;
	uint32_t edi;
	uint32_t ebp;
};

// old_sp points to where we should save the outgoing stack pointer, so it
// itself has to be a pointer. defined in asm/swtch.S
void swtch(virt_addr_t *old_sp, virt_addr_t new_sp, phys_addr_t new_cr3);

// snapshot the caller into `child` (see fork()). returns 1 in the parent and
// 0 in the child. defined in asm/swtch.S
int clone_parent_base_frame(struct process *child);

/* make a copy of the parent's stack, rebased into the child's stack space */
void clone_stack_rebased(struct process *child, virt_addr_t frame);

// clone_parent_base_frame()'s frame, lowest address first: edi esi ebx ebp. the
// saved frame pointer is the start of the chain fork_copy_stack() rebases
#define FORK_FRAME_FP_SLOT 3

void ctx_switch(struct process *new);

#endif
