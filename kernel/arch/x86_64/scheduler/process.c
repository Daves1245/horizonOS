#include <kheap.h>
#include <string.h>
#include <x86_64/scheduler/process.h>
#include <log.h>
#include <kernel/panic.h>

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

struct process *myproc() {
    // not important (?) in single core,
    // but in multicore, guards against mycpu()
    // changing value between our capture of *cpu,
    // and retrieving cpu->task (if the current process
    // was asigned to a new cpu between these instructions).
    // don't think, but not sure that return mycpu()->task
    // fixes this at all TODO(nice to know).
    disable_interrupts();
    struct cpu *cpu = mycpu();
    struct process *ret = cpu->task;
    enable_interrupts();
    return ret;
}

struct cpu *mycpu() {
    return &cpus[0];
}

void __init() {
    struct process *init = (struct process *) kmalloc(sizeof(*init));
    if (!init) {
        panic("could not allocate memory for init");
    }

    init->pid = __glbl_pid++;
    memcpy(init->name, "init", strlen("init"));
    init->parent = NULL;
    virt_addr_t stack = kmalloc_a(KSTACK_SIZE);
    if (!stack) {
        panic("could not allocate stack for init");
    }
    // TODO unmap guard page on kstack. for now, we overcompensate
    // with a largened stack to avoid overflows. see notes
    // at process.h
    init->kstack = stack;
    init->state = READY;

    // TODO(memory): the current paging setup reads the
    // current cr3 value, instead of filling it in with
    // something provided.
    init->pagetable = (struct page_table_t) {0};
    init->context = (struct context) {0};

    cpus[0].task = init;
}

struct process *aprocess() {
    struct process *ret = (struct process *) kmalloc(sizeof(*ret));
    if (!ret) {
        panic("aprocess: unable to allocate memory for process\n");
    }
    ret->pid = __glbl_pid++;
    ret->kstack = kmalloc_a(sizeof(KSTACK_SIZE));
    if (!ret->kstack) {
        panic("aprocess: could not allocate process stack\n");
    }
    ret->state = READY;
    // TODO again, the current paging setup reads the
    // current cr3 value. this way, all processes have
    // identical address space
    ret->pagetable = (struct page_table_t) {0};
    ret->context = (struct context) {0};
    return ret;
}

void enable_interrupts() {
    int old_if = read_rflags() & FL_IF;
    struct cpu *c = mycpu();
    if (!c->noff && c->intena) {
        asm volatile("sti");
    }
}

void disable_interrupts() {
    struct cpu *c = mycpu();
    c->noff++;
    asm volatile("cli");
}

int fork(const char *name) {
    struct process *parent = myproc();
    struct process *child = (struct process *) kmalloc(sizeof(*child));

    child->pid = __glbl_pid++;
    memcpy(child->name, name, strlen(name));

    child->parent = parent;
    child->state = READY;

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
