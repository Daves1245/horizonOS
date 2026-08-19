#include <x86_64/scheduler/scheduler.h>
#include <x86_64/scheduler/process.h>

// old_rsp points to where we should get the last stack
// pointer, so it itself should be a pointer
extern void swtch(uint64_t *old_rsp, uint64_t new_rsp, uint64_t new_cr3);

void ctx_switch(struct process *new) {
    // TODO(multicore)

    uint64_t flags = irq_save();

    struct process *old = myproc();

    cpus[0].task = new;

    swtch(&old->context.rsp, new->context.rsp, new->context.cr3);

    irq_restore(flags);
}

void scheduler() {
    struct process *p = &processes[0];

    while (1) {
        /* avoid deadlock if all other processes are waiting */
        // this is safe since once we're in the scheduler
        // we're guaranteed that IF is 1 for all processes. we
        // don't switch out while we're in a critical section.
        asm volatile("cli");
        asm volatile("sti");

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
