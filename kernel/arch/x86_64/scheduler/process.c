#include <kheap.h>
#include <string.h>
#include <x86_64/scheduler/process.h>
#include <x86_64/scheduler/scheduler.h>
#include <log.h>
#include <kernel/panic.h>

#include <halt.h>

struct cpu cpus[NUM_CPUS];
uint8_t __glbl_pid;
struct process processes[NUM_PROCESSES];

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

// build the kernel stack (and matching saved context) a brand new
// process needs to survive its first swtch() into it. swtch()
// always ends with pop r15; pop r14; pop r13; pop r12;
// pop rbx; pop rbp; ret
// which is fine for a process that has been swtch()'d *out* before,
// since that push/pop pattern is symmetric and the final ret lands
// back on whatever real return address swtch() was called from. a
// process that has never run has no such frame lying around, so we
// make one by hand. save the registers, followed by `entry`
// as the fake return address for the final ret to land on.
static void init_stack(struct process *p, void (*entry)(void)) {
    virt_addr_t stack = kmalloc_a(KSTACK_SIZE);
    if (!stack) {
        panic("init_stack: could not allocate stack for process");
    }
    p->addrspace.stack_base = stack;
    p->addrspace.stack_end = stack + KSTACK_SIZE;

    // one past the highest addressable word in this region
    virt_addr_t *stack_p = (virt_addr_t *) (stack + KSTACK_SIZE);

    // see `swtch()` (swtch.S), a newly initialized process
    // needs this boostrapped stack. the trick lies
    // in ret jumping us into the entry point.
    //
    // swtch() pushes rbp first and r15 last, so r15 ends up deepest.
    // the pops then walk back *up* (pop reads at rsp, then raises it),
    // which means the frame has to be built with r15 at the lowest
    // address and the return address at the highest, and rsp has to
    // start at the bottom of the frame rather than the top.
    stack_p[-1] = (virt_addr_t) entry; // ret to here (highest)
    stack_p[-2] = 0; // rbp (0 terminates a frame-pointer walk)
    stack_p[-3] = 0; // rbx
    stack_p[-4] = 0; // r12
    stack_p[-5] = 0; // r13
    stack_p[-6] = 0; // r14
    stack_p[-7] = 0; // r15 (lowest, popped first)

    p->context = (struct context) {0};
    p->context.rsp = (virt_addr_t) &stack_p[-7];
    // NOTE(abi): `ret` leaves rsp 16-aligned at `entry`, where the SysV
    // ABI wants rsp % 16 == 8 (what a real `call` leaves behind). that
    // only matters for 16-byte-aligned SSE accesses, and the kernel is
    // built -mno-sse, so leave it. if SSE is ever enabled, shift the
    // whole frame down one word so the return address sits 16-aligned.
    // TODO(memory): all processes currently share the same address
    // space, so just reuse whatever's active right now.
    p->context.cr3 = read_cr3();
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
    uint64_t flags = irq_save();

    // TODO(userspace): once processes carry a real entry point, jump
    // to it here instead. for now there's nothing process-specific
    // to run yet, so just return to the scheduler (it will pick a new process to run).
    scheduler();
    irq_restore(flags);
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

void __init() {
    // init lives in the process table like everything else, so the
    // scheduler can actually see it -- it used to be kmalloc'd and
    // reachable only through cpus[0].task, which meant scheduler()
    // (it only ever scans processes[]) could switch away from init
    // but never back to it.
    struct process *init = &processes[0];

    init->pid = __glbl_pid++;
    memcpy(init->name, "init", strlen("init") + 1); // +1 for the NUL
    init->parent = NULL;
    virt_addr_t stack = kmalloc_a(KSTACK_SIZE);
    if (!stack) {
        panic("could not allocate stack for init");
    }
    // TODO unmap guard page on kstack. for now, we overcompensate
    // with a largened stack to avoid overflows. see notes
    // at process.h
    init->addrspace.stack_base = stack;
    init->addrspace.stack_end = stack + KSTACK_SIZE;
    // RUNNING, not READY: init is the process executing right now, and
    // it has no saved frame yet (context.rsp is still 0 until the first
    // swtch() *out* of it fills it in). marking it READY would let
    // scheduler() pick it and ctx_switch() to itself, loading rsp = 0.
    init->state = RUNNING;

    // TODO(memory): the current paging setup reads the
    // current cr3 value, instead of filling it in with
    // something provided.
    init->context = (struct context) {0};
    init->context.cr3 = read_cr3();

    cpus[0].task = init;
}

struct process *alloc_process() {
    struct process *ret = (struct process *) kmalloc(sizeof(*ret));
    if (!ret) {
        panic("aprocess: unable to allocate memory for process\n");
    }
    ret->pid = __glbl_pid++;
    ret->parent = NULL;
    ret->state = READY;
    // TODO again, the current paging setup reads the
    // current cr3 value. this way, all processes have
    // identical address space

    init_stack(ret, forkret);
    return ret;
}

int fork(const char *name) {
    struct process *parent = myproc();
    struct process *child = alloc_process();

    memcpy(child->name, name, strlen(name));
    child->parent = parent;

    int found = 0;
    for (struct process *p = processes; p < &processes[NUM_PROCESSES]; p++) {
        if (p->state == UNUSED) {
            found = 1;
            memcpy(p, child, sizeof(*child));
            break;
        }
    }

    if (!found) {
        printk(KERN_DEBUG, "fork: process limit hit");
        return -1;
    }

    return child->pid;
}
