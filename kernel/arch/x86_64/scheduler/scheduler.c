#include <x86_64/scheduler/scheduler.h>
#include <x86_64/scheduler/process.h>
#include <x86_64/scheduler/lock.h>

// old_rsp points to where we should get the last stack
// pointer, so it itself should be a pointer
extern void swtch(uint64_t *old_rsp, uint64_t new_rsp, uint64_t new_cr3);

#define NUM_PRIORITY_LEVELS 3

// Multilevel feedback queue
struct list_head mlfq[NUM_PRIORITY_LEVELS];

// 
struct list_head ready;

int mlfq_lock;

/* prepare data structures (mlfq and ready list) */
void init_scheduler() {

    INIT_LIST_HEAD(&ready);

    for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        INIT_LIST_HEAD(&mlfq[i]);
    }
}

void ctx_switch(struct process *new) {
    // TODO(multicore)

    uint64_t flags = irq_save();

    struct process *old = myproc();

    cpus[0].task = new;

    swtch(&old->context.rsp, new->context.rsp, new->context.cr3);

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
        acquire_lock(&mlfq_lock);
        for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
            // careful functions: https://docs.kernel.org/core-api/list.html#c.list_empty_careful
            // tests whether a list is empty _and_ checks that no other CPU might be in the process of modifying either member (next or prev)
            if (!list_empty_careful(&mlfq[i])) {
                list_for_each_entry_safe(p, tmp, &mlfq[i], sched) {
                    // list_del_init but also checks the same as the above for empty
                    list_del_init_careful(&p->sched);
                    struct cpu *cpu = mycpu();
                    cpu->task = p;
                    release_lock(&mlfq_lock);
                    // TODO memory leak race condition - if we interrupt here, we lose
                    // the pointer to the process that just ran (p).
                    ctx_switch(p);

                    list_add(&p->sched, &ready);
                    found = 1;
                }
            }
            if (found) break;
        }
        release_lock(&mlfq_lock);

        int found = 0;
        // TODO(multicore)
        // to support more than one cpu, swtch() must take in which cpu
        // to perform the switch on, and each process must also have a
        // lock to prevent data races between multiple schedulers.
        // each cpu must run its own scheduler(). for a multicore system,
        // we must wait at the end for an interrupt at the end from another
        // core.
        for (; !found ;) {
            if (p->state == READY) {
                p->state = RUNNING;
                ctx_switch(p);
                if (p->state == RUNNING) {
                    p->state = READY;
                }
                found = 1;
            }

            if (p == &processes[NUM_PROCESSES]) {
                p = &processes[0];
            }
        }
    }
}

void yield() {
    struct process *p = myproc();
    p->state = READY;
    scheduler();
}
