/*
 * Kernel context switching.
 */
#ifndef X86_64_SWITCH_H
#define X86_64_SWITCH_H

#include <stdint.h>
#include <asm/mm.h>

struct process;

struct context {
	/* stack frame */
	virt_addr_t sp;
	//virt_addr_t rbp; // not necessary

	int intena;
	int noff;

	/* general purpose registers (not callee-saved between functions) */
	uint64_t rbx;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;

	// ffu, mmx, see registers etc. (not currently used)
};

// old_sp points to where we should save the outgoing stack pointer, so it
// itself has to be a pointer. defined in asm/swtch.S
void swtch(virt_addr_t *old_sp, virt_addr_t new_sp, phys_addr_t new_cr3);

// snapshot the caller into `child` (see fork()). returns 1 in the parent and
// 0 in the child. defined in asm/swtch.S
int clone_parent_base_frame(struct process *child);

// used in fork(). walks and copies the parent's stack bottom up into the
// stack of the child.
void clone_stack_rebased(struct process *child, virt_addr_t frame);

// clone_parent_base_frame()'s frame, lowest address first: r15 r14 r13 r12 rbx rbp. the
// saved frame pointer is the start of the chain fork_copy_stack() rebases
#define FORK_FRAME_FP_SLOT 5

void ctx_switch(struct process *new);

#endif
