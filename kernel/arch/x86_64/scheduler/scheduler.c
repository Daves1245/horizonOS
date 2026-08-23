#include <x86_64/scheduler/scheduler.h>
#include <x86_64/scheduler/process.h>
#include <x86_64/scheduler/lock.h>

// old_rsp points to where we should get the last stack
// pointer, so it itself should be a pointer
extern void swtch(uint64_t *old_rsp, uint64_t new_rsp, uint64_t new_cr3);

#define NUM_PRIORITY_LEVELS 3

// Multilevel feedback queue
struct list_head mlfq[NUM_PRIORITY_LEVELS];

struct list_head ready;

lock_t mlfq_lock;

/* prepare data structures (mlfq and ready list) */
void init_scheduler() {

    INIT_LIST_HEAD(&ready);

    for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        INIT_LIST_HEAD(&mlfq[i]);
    }
}

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

    swtch(&old->context.rsp, new->context.rsp, (virt_addr_t) new->pagetable);

    irq_restore(flags);
}

void scheduler() {
    while (1) {
        /* avoid deadlock if all other processes are waiting */
        // this is safe since once we're in the scheduler
        // we're guaranteed that IF is 1 for all processes. we
        // don't switch out while we're in a critical section.
        //
        // TODO: claude marks this as not being correct. revisit
        // xv86 and figure out why this works. perhaps adding
        // a memory clobber in case it's related to flushing
        // registers?
        asm volatile("cli");
        asm volatile("sti");

        // TODO figure out if it's possible only to lock around
        // actually modifying the queue (removing an entry, running it, and adding it back)

        struct process *p;
        struct process *tmp; // temporary storage  for safe iteration (deleting nodes while iterating)
        int found = 0;
        spinlock(&mlfq_lock);
        for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
            // careful functions: https://docs.kernel.org/core-api/list.html#c.list_empty_careful
            // tests whether a list is empty _and_ checks that no other CPU might be in the process of modifying either member (next or prev)
            if (!list_empty_careful(&mlfq[i])) {
                list_for_each_entry_safe(p, tmp, &mlfq[i], sched) {
                    // list_del_init but also checks the same as the above for empty
                    list_del_init_careful(&p->sched);
                    struct cpu *cpu = mycpu();
                    cpu->task = p;
                    release(&mlfq_lock);
                    // TODO memory leak race condition - if we interrupt here, we lose
                    // the pointer to the process that just ran (p).
                    ctx_switch(p);

                    list_add(&p->sched, &ready);
                    found = 1;
                }
            }
            if (found) break;
        }
        release(&mlfq_lock);
    }
}

void yield() {
    struct process *p = myproc();
    p->state = READY;
    scheduler();
}
