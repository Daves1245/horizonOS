#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <asm/paging.h>
#include <asm/mm.h>
#include <asm/switch.h>

#define NUM_PROCESSES 10
#define NUM_CPUS 2

/* +1 for guard page. the idea being we unmap
 * the last page on the stack (grows down), and
 * add logic to the page fault interrupt to detect
 * if the page is unmapped and at some offset
 * from a kernel stack base. if so, say something like
 * "stack overflow on <pid>". i believe it's sufficient
 * to clear the PRESENT bit on the page, and invalidate_page().
 * for now, we overcompensate with a largened stack. 16 KiB
 * should be more than enough, it's what linux uses (although
 * linux handles thousands+ (?) processes at a time, so there's
 * a memory pressure on smaller devices).
 * TODO(fact check)
 * TODO(later)
 */
#define KSTACK_PAGES 32
#define KSTACK_SIZE ((KSTACK_PAGES + 1) * PAGE_SIZE)
// TODO(userspace)
// track TSS and load new SS and ESP on context switch for switching
// to a higher privilege level

enum process_state {
	UNUSED, // (i believe) c standard guarantees this will
	// be default-0-initialized, so we include
	// this UNUSED value to prevent the scheduler
	// from accidentally using empty (bss'ed) data.
	READY,
	RUNNING,
	SLEEPING,
	ZOMBIE
};

struct cpu {
	int cpuid;
	int intena;
	int noff;

	// currently running process
	struct process *task;

	// the scheduler runs as a process with a stack of its own, so that control
	// can be handed back to it without borrowing the stack of whoever was
	// interrupted. it is not on the mlfq -- it is what services the mlfq.
	struct process *scheduler_proc;
};

void init_process(void);

extern struct cpu cpus[NUM_CPUS];

extern uint8_t __glbl_pid;

struct addrspace {
	phys_addr_t page_table_base; // pass to cr3

	// TODO(cleanup) ideally, we'd like to add a guard page
	// at the end of the stack. we currently do that, but we don't
	// have logic within the page fault handler to detect this.
	// for now, we can just have two pointers, and compare if
	// our accesses ever exceed the end of the stack. this
	// adds on an unnecessary, expensive operation for now!

	struct list_head vm_list; // linked list of vm_region
};

struct process {
	uint8_t pid;
	char name[32];

	struct process *parent;
	struct addrspace addrspace;

	// entry into multilevel feedback queue in scheduler
	struct list_head sched;

	enum process_state state;
	phys_addr_t cr3;
	struct context context;

	// entry point for new execution
	void (*entry)(void);
};

void sched(void);
int fork(const char *name, void (*entry)(void), int level);

struct process *myproc();
struct cpu *mycpu();

#endif
