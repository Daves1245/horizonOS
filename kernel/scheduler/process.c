#include <kheap.h>
#include <string.h>
#include <kernel/scheduler/process.h>
#include <kernel/scheduler/scheduler.h>
#include <log.h>
#include <kernel/panic.h>
#include <serial.h>
#include <time.h>
#include <asm/irqflags.h>
#include <asm/switch.h>

#include <halt.h>

struct cpu cpus[NUM_CPUS];
uint8_t __glbl_pid;

// the process table
static struct process processes[NUM_PROCESSES];

static struct process *alloc_process(const char *name, void (*entry)(void));
void exit_process(void);

// dummy location to dump this execution environment's
// %rsp when we bootstrap ourselves into the init() process
static struct process bootstrap_proc;

/*
 * alloc_stack: allocate a kernel stack and the vm_region describing it.
 *
 * The stack is left empty. init_stack() seeds an initial frame on it, and
 * fork() copies the parent's frames into it.
 */
static struct vm_region *alloc_stack(void) {
	/*
	 * TODO(memory): add a frame allocator to upgrade from the current
	 * bump allocator.
	 */
	virt_addr_t stack = kmalloc_a(KSTACK_SIZE);
	struct vm_region *vma_stack =
		(struct vm_region *)kmalloc(sizeof(*vma_stack));

	if (!stack || !vma_stack) {
		panic("alloc_stack: could not allocate stack for process");
	}

	vma_stack->flags =
		FL_VM_READ |
		FL_VM_WRITE; // not used, just here for the fun of the game
	vma_stack->type = STACK;

	vma_stack->start = stack;
	vma_stack->end = stack + KSTACK_SIZE;

	// guard page (stack grows down, so we have to guard the first page)
	phys_addr_t guard_frame = unmap_page(vma_stack->start, read_cr3());

	if (!guard_frame) {
		panic("alloc_stack: guard page was not mapped in the first place");
	}

	return vma_stack;
}

// build the kernel stack (and matching context saved). brand new
// processes needs to survive its first swtch() into it. swtch()
// always ends with pop r15; pop r14; pop r13; pop r12; pop rbx;
// pop rbp; ret
//
// which is fine for a process that has been swtch()'d *out* before,
// since that push/pop pattern is symmetric and the final ret lands
// back on whatever real return address swtch() was called from. a
// process that has never run has no such frame, so we make one manually.
// save the registers, followed by `entry` as the return address for the
// final ret to land on. symmetry is the point here.
static struct vm_region *init_stack(struct process *p, void (*entry)(void)) {
	struct vm_region *vma_stack = alloc_stack();

	// one past the highest addressable word in this region
	virt_addr_t *stack_p = (virt_addr_t *)vma_stack->end;

	// see `swtch()` (swtch.S), a newly initialized process
	// needs this boostrapped stack. the trick lies
	// in ret jumping us into the entry point.
	//
	// swtch() pushes rbp first and r15 last, so r15 ends up deepest
	// in the stack. the pops then walk back *up*
	// (pop reads at rsp, then raises it), which means the frame has
	// to be built with r15 at the lowest address and the return
	// address at the highest, and rsp has to start at the bottom of
	// the frame rather than the top.

	// the SysV AMD64 ABI wants %rsp 16-byte aligned at a `call`, so the
	// return address that `call` pushes leaves the callee looking at an %rsp
	// of 8 (mod 16) on its first instruction. swtch()'s `ret` pops our fake
	// return address and leaves %rsp at &stack_p[-1], and stack_p is page
	// aligned, so &stack_p[-1] is the slot that pushes the return address.
	// hence the padding word: the frame starts one. we build with -mno-sse
	// -msoft-float, so nothing in the entry point emits the 16-byte-aligned
	// `movaps` that would actually fault on this today
	stack_p[-1] = 0; // padding, leaves `entry` looking at rsp % 16 == 8
	stack_p[-2] = (virt_addr_t)entry; // ret to here
	stack_p[-3] = 0; // rbp (0 terminates a frame-pointer walk)
	stack_p[-4] = 0; // rbx
	stack_p[-5] = 0; // r12
	stack_p[-6] = 0; // r13
	stack_p[-7] = 0; // r14
	stack_p[-8] = 0; // r15 (lowest, popped first)

	p->context.sp = (virt_addr_t)&stack_p[-8]; // r15

	return vma_stack;
}

// the first thing a brand new process runs, landed on via swtch()'s
// final `ret` (see init_stack() above). ctx_switch() re-enables
// interrupts right after its call to swtch() returns; a fresh
// process skips straight past that call and lands here instead, so
// it has to redo that step itself before anything else.

// AMD64 ABI 1.0 - March 12, 2025
// "the standard calling sequence requirements apply only to global functions"
// this should be private, but i'm too lazy to look up whether gcc
// handles private functions differently. for now, just keep it
// private
// TODO(cleanup)
void forkret(void) {
	struct process *p = myproc();

	// fork() brings us back here after swtch(), but clears IF before all
	// of that. we restore the flag as it was before the fork.
	irq_restore(FL_IF);

	p->entry();

	// the entry point returned. there is nothing to return to: swtch()'s
	// ret already consumed the only address this stack ever had.
	exit_process();
}

// give up the cpu for good. the scheduler drops zombies rather than putting
// them back on the queue, so this never comes back.
// TODO(cleanup) nothing reaps these yet, stack and the table slot stay
// held until someone writes wait().
void exit_process(void) {
	struct process *p = myproc();

	p->state = ZOMBIE;

	ctx_switch(mycpu()->scheduler_proc);
	panic("exit_process: scheduler switched back into a zombie");
}

struct process *myproc(void) {
	// not important (?) in single core,
	// but in multicore, guards against mycpu()
	// changing value between our capture of *cpu,
	// and retrieving cpu->task (if the current process
	// was asigned to a new cpu between these instructions).
	// don't think, but not sure that return mycpu()->task
	// fixes this at all TODO(nice to know).
	uint64_t irq_flags = irq_save();
	struct cpu *cpu = mycpu();
	struct process *ret = cpu->task;

	irq_restore(irq_flags);
	return ret;
}

struct cpu *mycpu(void) {
	return &cpus[0];
}

#define ABAB_TURNS 10

// the two halves of the ABAB test. each one prints its letter and hands the
// cpu straight back, so the output is a direct transcript of the order the
// scheduler picked them in.
static void proc_a(void) {
	for (int i = 0; i < ABAB_TURNS; i++) {
		printk(KERN_RAW, "A");
		yield();
	}

	printk(KERN_RAW, "\n");
	printk(KERN_INFO, "A: done.\n");
}

static void proc_b(void) {
	for (int i = 0; i < ABAB_TURNS; i++) {
		printk(KERN_RAW, "B");
		yield();
	}

	printk(KERN_INFO, "B: done.\n");
}

void init(void) {
	printk(KERN_INFO, "init: pid %d, on its own kernel stack\n",
	       myproc()->pid);

	// KERN_RAW is below the default threshold, so the A/B ticks would be
	// filtered out. nothing after this point needs the filtering.
	set_log_level(KERN_RAW);

	// start a above b, and they drop in the MLFQ until they're in the bottom
	// level, at which point they're RR'ing, still in alternating order.
	// the children come back out of fork() right here, so each one has to
	// peel off before it runs the rest of init()
	int a = fork("A", 0);

	if (a == 0) {
		proc_a();
		exit_process();
	}

	int b = fork("B", 1);

	if (b == 0) {
		proc_b();
		exit_process();
	}

	if (a < 0 || b < 0) {
		panic("init: could not fork the A/B test processes");
	}

	printk(KERN_INFO, "init: forked A (pid %d) and B (pid %d)\n", a, b);

	// the scheduler process is not on the mlfq since queueing it would have it pick itself.
	// doing a swtch() with old == new saves the outgoing %rsp in the context it will restore.
	struct process *sched_p = alloc_process("scheduler", scheduler);

	if (!sched_p) {
		panic("init: could not allocate the scheduler process\n");
	}

	printk(KERN_INFO, "init: scheduler process created\n");

	mycpu()->scheduler_proc = sched_p;

	printk(KERN_INFO, "init: starting scheduler\n");

	// last bit: hook the scheduler into the end of every interrupt
	register_interrupt_handler(TIMER_INT_NO, timer_interrupt_handler_sched);

	// hand the cpu over. init is on the mlfq, so the scheduler switches back
	// here when it comes round to us.
	ctx_switch(sched_p);

	// init is the idle task from here. it has to keep handing the cpu back
	// rather than halting: nothing preempts us yet, so a halt here would
	// strand every other process still on the queue.
	//
	// TODO(bugs):
	// bugs!
	//
	// if we remove this, we enter lots of races when running QEMU.
	// we might hang before starting the ABAB test, miss a newline print
	// for some reason, or have nothing happen. but for now we should be able
	// to 'exit' init (for now, later we'll loop back into kernel code and setup other
	// things)
	for (;;) {
		yield();
	}
}

// the 'init' process
void init_process(void) {
	// init takes a table slot like any other process. alloc_process() does
	// the stack, the name, the (empty) addrspace list and a cr3 inherited
	// from the parent.
	struct process *init_p = alloc_process("init", init);

	if (!init_p) {
		panic("could not allocate init process");
	}

	// but init gets its own top-level table rather than the bootloader's.
	// only that table is copied, so every lower level is still shared.
	pgtable_t *bootstrap =
		(pgtable_t *)phys_to_virt(PAGE_GET_ADDR(read_cr3()));
	pgtable_t *pgtable = (pgtable_t *)kmalloc_a(sizeof(pgtable_t));

	memcpy(pgtable, bootstrap, sizeof(pgtable_t));
	init_p->pml4_addr = virt_to_phys((virt_addr_t) pgtable);

	// init lives on the mlfq like everything else, even though we bootstrap
	// straight into it below instead of letting the scheduler pick it up. it
	// goes on the *lowest* level: it is the idle task, so anything else that
	// is runnable should be picked ahead of it.
	sched_enqueue(init_p, NUM_PRIORITY_LEVELS - 1);

	// bootstrap: the context we are running on is abandoned here. point the
	// cpu at the throwaway process first so swtch() has somewhere to save it
	// through ctx_switch() isntalling init_p as cpus[0].task itself.
	cpus[0].task = &bootstrap_proc;
	init_p->state = RUNNING;

	ctx_switch(init_p);

	// nothing past this point
}

/*
 * claim a free slot in the process table and bring it up to the point where
 * the scheduler can switch into it. the process isn't queued, it
 * becomes reachable to the scheduler only once the caller enqueues it, which
 * is what lets us finish initializing it without racing anyone.
 */
// @return the new process, or NULL when the table is full
static struct process *alloc_process(const char *name, void (*entry)(void)) {
	// TODO(multicore) this wants a real ptable lock, cli is not enough there
	uint64_t flags = irq_save();

	struct process *p = NULL;

	for (struct process *it = processes; it < &processes[NUM_PROCESSES]; it++) {
		if (it->state == UNUSED) {
			p = it;
			break;
		}
	}

	if (!p) {
		irq_restore(flags);
		return NULL;
	}

	// .bss hands us zeros the first time round, but a recycled slot is the
	// corpse of a dead process. start from a known state either way.
	*p = (struct process){ 0 };

	p->pid = __glbl_pid++;
	p->parent = NULL;
	p->state = READY;
	// caller's problem to keep names under sizeof(p->name)
	memcpy(p->name, name, strlen(name) + 1);

	INIT_LIST_HEAD(&p->addrspace.vm_list);

	p->pml4_addr = cr3_to_pml4_addr(read_cr3());

	// every process starts in forkret(), which then calls p->entry(). that
	p->entry = entry;

	struct vm_region *stack_region = init_stack(p, forkret);

	list_add(&stack_region->node, &p->addrspace.vm_list);

	irq_restore(flags);
	return p;
}

struct process *alloc_process_no_entry(const char *name) {
	uint64_t flags = irq_save();

	struct process *p = NULL;

	for (struct process *it = processes; it < &processes[NUM_PROCESSES];
			it++) {
		if (it->state == UNUSED) {
			p = it;
			break;
		}
	}

	if (!p) {
		irq_restore(flags);
		return NULL;
	}

	// .bss hands us zeros the first time round, but a recycled slot is the
	// corpse of a dead process. start from a known state either way.
	*p = (struct process){ 0 };

	p->pid = __glbl_pid++;
	p->parent = NULL;
	p->state = READY;
	// caller's problem to keep names under sizeof(p->name)
	memcpy(p->name, name, strlen(name) + 1);

	INIT_LIST_HEAD(&p->addrspace.vm_list);

	// TODO(addrspace) all processes currently share the same address space.
	// see the documentaton around copy_stack_rebased().
	p->pml4_addr = PAGE_GET_ADDR(read_cr3());

	// no frame on it yet: fork() fills it with a copy of the parent's
	struct vm_region *stack_region = alloc_stack();

	list_add(&stack_region->node, &p->addrspace.vm_list);

	irq_restore(flags);
	return p;
}

static struct vm_region *find_stack(struct process *p) {
	struct vm_region *r;

	list_for_each_entry(r, &p->addrspace.vm_list, node) {
		if (r->type == STACK) {
			return r;
		}
	}

	panic("find_stack: process has no stack region");
}

/*
 * clone_stack_rebased: copy a parent's stack into a child address space
 *
 * The internals of copy_stack (swtch.S). The assembly only creates a snapshot
 * of the outer frame of the parent that the context switch can return into.
 *
 * There is only one address space so far. For a number of reasons:
 *
 * 1) We don't have userspace yet.
 * 2) init copies over cr3 from the limine bootloader
 * 3) init_process() does a shallow copy on each fork
 *
 * Number 1 is an issue because in kernelspace, every process shares the same
 * address space. Until we get userspace, we're stuck manually cloning each
 * stack on a fork.
 *
 * see TODO(addrspace).
 */

void clone_stack_rebased(struct process *child, virt_addr_t frame) {
	virt_addr_t parent_stack_top = find_stack(myproc())->end;
	virt_addr_t child_stack_top = find_stack(child)->end;
	// wraps when the child sits lower, which should be fine
	virt_addr_t delta = child_stack_top - parent_stack_top;
	virt_addr_t child_frame = frame + delta;

	memcpy((void *) child_frame, (void *)frame, parent_stack_top - frame);

	// init_stack() seeds the bottom frame with a 0 frame pointer, which falls
	// outside the parent's stack and ends the walk.
	virt_addr_t *slot = (virt_addr_t *) child_frame + FORK_FRAME_FP_SLOT;

	while (*slot >= frame && *slot < parent_stack_top) {
		*slot += delta;
		slot = (virt_addr_t *) *slot;
	}

	child->context.sp = child_frame;
}

/*
 *
 * unix-like fork: duplicate a process.
 *
 *
 * The child process is a copy of the parent and returns to the same point
 * as the parent, through its own stack. Stack walking and copying is done
 * to replicate the original structure.
 *
 * TODO(fork) copy the address space too once there is one per process.
 *
 * @param name Name of the child process, used internally for debugging.
 * @param level Initial priority level to start the child process. See the MLFQ scheduler for details
 * @return The child's pid in the parent, 0 in the child, or -1 if the
 * process table is full
 */
int fork(const char *name, int level) {
	/*
	 * Fairly straightforward: Allocate an address space, reserve a stack (this
	 * includes making a guard page for overflows), copy over the callee's stack
	 * through a walk, and enqueue into the scheduler.
	 */

	// disable (save) interrupts
	uint64_t flags = irq_save();

	struct process *parent = myproc();
	struct process *child = alloc_process_no_entry(name);

	if (!child) {
		irq_restore(flags);
		printk(KERN_DEBUG, "fork: process limit hit\n");
		return -1;
	}

	child->parent = parent;

	if (clone_parent_base_frame(child) == 0) {
		// we are the child. we came back from swtch(), IF is clear and
		// we skipped the re-enable on the far side of ctx_switch().
		// `flags` is the parent's, copied over with the rest of the frame.
		irq_restore(flags);
		return 0;
	}

	sched_enqueue(child, level);

	irq_restore(flags);
	return child->pid;
}
