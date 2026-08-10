#include <x86_64/scheduler/scheduler.h>
#include <x86_64/scheduler/process.h>

// old_rsp points to where we should get the last stack
// pointer, so it itself should be a pointer
extern void swtch(uint64_t *old_rsp, uint64_t new_rsp, uint64_t new_cr3);

void ctx_switch(struct process *new) {
    // TODO(multicore)

    // push registers onto stack, reset rsp, and pop
    // new registers

    disable_interrupts();

    struct process *old = myproc();

    cpus[0].task = new;

    swtch(&old->context.rsp, new->context.rsp, new->context.cr3);

    enable_interrupts();
}

void scheduler() {
    struct process *p;

    while (1) {
        /* avoid deadlock if all other processes are waiting */
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
        for (p = &processes[0]; p < &processes[NUM_PROCESSES]; p++) {
            if (p->state == READY) {
                p->state = RUNNING;
                ctx_switch(p);
                found = 1;
            }
        }

        if (!found) {
            // asm volatile ("hlt");
        }
    }
}

void yield() {
    struct process *p = myproc();
    p->state = READY;
    scheduler();
}
