#include <kheap.h>
#include <string.h>
#include <x86_64/scheduler/process.h>
#include <x86_64/scheduler/scheduler.h>
#include <log.h>
#include <kernel/panic.h>
#include <drivers/serial.h>
#include <drivers/timer.h>

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

// TODO move to cpu.c later maybe for organization?
static inline uint64_t read_rflags() {
  uint64_t flags;
  // flags register isn't addressable, so we push and
  // pop the flag register and store the result into `flags`
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
uint64_t irq_save() {
  uint64_t flags = read_rflags() & FL_IF;
  asm volatile("cli" ::: "memory");
  return flags;
}

void irq_restore(uint64_t flags) {
  if (flags & FL_IF) {
    asm volatile("sti" ::: "memory");
  }
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
  virt_addr_t stack = kmalloc_a(KSTACK_SIZE);
  struct vm_region *vma_stack = (struct vm_region *) kmalloc(sizeof(*vma_stack));

  if (!stack || !vma_stack) {
    panic("init_stack: could not allocate stack for process");
  }

  // one past the highest addressable word in this region
  virt_addr_t *stack_p = (virt_addr_t *) (stack + KSTACK_SIZE);

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
  stack_p[-2] = (virt_addr_t) entry; // ret to here
  stack_p[-3] = 0; // rbp (0 terminates a frame-pointer walk)
  stack_p[-4] = 0; // rbx
  stack_p[-5] = 0; // r12
  stack_p[-6] = 0; // r13
  stack_p[-7] = 0; // r14
  stack_p[-8] = 0; // r15 (lowest, popped first)

  vma_stack->flags = FL_VM_READ | FL_VM_WRITE; // not used, just here for the fun of the game
  vma_stack->type = STACK;

  vma_stack->start = stack;
  vma_stack->end = (virt_addr_t) stack_p;

  p->context.rsp = (virt_addr_t) &stack_p[-8]; // r15

  // guard page (stack grows down, so we have to guard the first page)
  phys_addr_t guard_frame = unmap_page(vma_stack->start, read_cr3());
  if (!guard_frame) {
    panic("init_stack: guard page was not mapped in the first place");
  }

  return vma_stack;
}

// the first thing a brand new process runs, landed on via swtch()'s
// final `ret` (see init_stack() above). ctx_switch() re-enables
// interrupts right after its call to swtch() returns; a fresh
// process skips straight past that call and lands here instead, so
// it has to redo that step itself before anything else.

// AMD64 ABI 1.0 – March 12, 2025
// "the standard calling sequence requirements apply only to global functions"
// this should be private, but i'm too lazy to look up whether gcc
// handles private functions differently. for now, just keep it
// private
// TODO(cleanup)
void forkret(void) {
  // ctx_switch() clears IF before swtch() and re-enables it on the far side
  // of that call. a brand new process never reaches that far side -- swtch()
  // drops it here instead -- so this is where it makes the step up.
  irq_restore(FL_IF);

  struct process *p = myproc();
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

struct process *myproc() {
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

struct cpu *mycpu() {
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

  printk(KERN_INFO, "B: done. \n");
}

void init() {
  printk(KERN_INFO, "init: pid %d, on its own kernel stack\n", myproc()->pid);

  // KERN_RAW is below the default threshold, so the A/B ticks would be
  // filtered out. nothing after this point needs the filtering.
  set_log_level(KERN_RAW);

  // start a above b, and they drop in the MLFQ until they're in the bottom
  // level, at which point they're RR'ing, still in alternating order.
  int a = fork("A", proc_a, 0);
  int b = fork("B", proc_b, 1);

  if (a < 0 || b < 0) {
    panic("init: could not fork the A/B test processes");
  }

  printk(KERN_INFO, "init: forked A (pid %d) and B (pid %d)\n", a, b);

  // the scheduler is a process with a kernel stack of its own,
  // rather than running on the stack of the process that happened
  // to call scheduler(). this makes the TODO work later: an ISR can hand
  // control back to a context that is not the interrupted process' own.
  //
  // it is not on the mlfq since queueing it would have it pick itself.
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
  // TODO(scheduler) once the timer ISR drives the scheduler this goes back
  // to being a hlt loop, and yield() can wait on ready instead of spinning.
  for (;;) {
    yield();
  }
}

// the 'init' process
void init_process() {
  // init takes a table slot like any other process. alloc_process() does
  // the stack, the name, the (empty) addrspace list and a cr3 inherited
  // from the parent.
  struct process *init_p = alloc_process("init", init);
  if (!init_p) {
    panic("could not allocate init process");
  }

  // but init gets its own top-level table rather than the bootloader's.
  // only the p4d is copied, so every lower level is still shared.
  p4d_t *bootstrap = (p4d_t *) phys_to_virt(PAGE_GET_ADDR(read_cr3()));
  p4d_t *p4d = (p4d_t *) kmalloc_a(sizeof(p4d_t));
  memcpy(p4d, bootstrap, sizeof(p4d_t));
  init_p->cr3 = virt_to_phys((virt_addr_t) p4d);

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
  *p = (struct process){0};

  p->pid = __glbl_pid++;
  p->parent = NULL;
  p->state = READY;
  // caller's problem to keep names under sizeof(p->name)
  memcpy(p->name, name, strlen(name) + 1);

  INIT_LIST_HEAD(&p->addrspace.vm_list);

  // TODO(addrspace) every process still inherits the address space it was
  // created in, so they all share one. a real fork has to copy the tables.
  // note this is the *address* bits only -- read_cr3() also returns
  // PWT/PCD, which have no business in a saved context.
  p->cr3 = PAGE_GET_ADDR(read_cr3());

  // every process starts in forkret(), which then calls p->entry(). that
  p->entry = entry;

  struct vm_region *stack_region = init_stack(p, forkret);
  list_add(&stack_region->node, &p->addrspace.vm_list);

  irq_restore(flags);
  return p;
}

// not a fork in the unix sense yet: the child does not resume where the
// parent is, it starts at `entry` and shares the parent's address space.
// TODO(fork) copy the address space and return 0 in the child once userspace
// exists and a child can inherit the parent's instruction pointer.
//
// @param level priority level to start the child on, 0 being the highest
// @return the child's pid, or -1 if the process table is full
int fork(const char *name, void (*entry)(void), int level) {
  struct process *parent = myproc();
  struct process *child = alloc_process(name, entry);

  if (!child) {
    printk(KERN_DEBUG, "fork: process limit hit\n");
    return -1;
  }

  child->parent = parent;

  // publish last: enqueueing is what makes the child visible to the
  // scheduler, so nothing may be left half-initialized past this point
  sched_enqueue(child, level);

  return child->pid;
}
