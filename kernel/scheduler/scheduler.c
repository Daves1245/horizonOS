#include <kernel/scheduler/scheduler.h>
#include <kernel/scheduler/process.h>
#include <include/lock.h>
#include <kernel/panic.h>
#include <asm/irqflags.h>
#include <asm/switch.h>

// Multilevel feedback queue
struct list_head mlfq[NUM_PRIORITY_LEVELS];

struct list_head ready;

lock_t mlfq_lock;

/* prepare data structures (mlfq and ready list) */
void init_scheduler(void) {
	// these live in .bss, so their next/prev come up NULL rather than
	// pointing at themselves
	INIT_LIST_HEAD(&ready);

	for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
		INIT_LIST_HEAD(&mlfq[i]);
	}

	mlfq_lock = INIT_LOCK;
}

/* queue a process on one priority level of the mlfq. tail insertion, so a
 * level is served round-robin in arrival order. */
void sched_enqueue(struct process *p, int level) {
	if (level < 0 || level >= NUM_PRIORITY_LEVELS) {
		panic("sched_enqueue: priority level out of range");
	}

	// the lock is held with interrupts off. nothing in an ISR touches the
	// mlfq today, but the moment the timer drives the scheduler, taking this
	// with IF set is a self-deadlock waiting to happen.
	uint64_t flags = irq_save();

	spinlock(&mlfq_lock);

	list_add_tail(&p->sched, &mlfq[level]);

	release(&mlfq_lock);
	irq_restore(flags);
}

/* a process that has had its turn drops one level in a MLFQ */
static int demote(int level) {
	if (level + 1 < NUM_PRIORITY_LEVELS) {
		return level + 1;
	}

	return level;
}

// we make the scheduler its own process with special properties
void scheduler(void) {
	while (1) {
		// we arrive here with IF clear, every time: ctx_switch() cleared it
		// on the way in and a process that yields to us hands us its cleared
		// flags. re-enable, or the first switch wedges the timer for good.
		asm volatile("sti" ::: "memory");

		struct process *p = NULL;
		int level = 0;

		// hold the queue only long enough to claim a process. running it
		// under the lock would mean handing the lock to whoever we switch to.
		uint64_t flags = irq_save();

		spinlock(&mlfq_lock);

		for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
			// careful functions: https://docs.kernel.org/core-api/list.html#c.list_empty_careful
			// tests whether a list is empty _and_ checks that no other CPU might be in the process of modifying either member (next or prev)
			if (list_empty_careful(&mlfq[i])) {
				continue;
			}

			// highest priority level with anything on it wins, and within a
			// level it is the oldest entry sched_enqueue() adds at the tail
			p = list_first_entry(&mlfq[i], struct process, sched);

			// list_del_init but also checks for other CPUs modifying this member
			list_del_init_careful(&p->sched);
			level = i;
			break;
		}

		release(&mlfq_lock);
		irq_restore(flags);

		if (!p) {
			// spin and look again.
			continue;
		}

		p->state = RUNNING;

		ctx_switch(p);

		// p handed the cpu back. it either yielded (still runnable) or died
		// in exit_process(). only a runnable process goes back on the queue --
		// re-queueing a zombie means switching into a dead stack.
		if (p->state == ZOMBIE) {
			continue;
		}

		sched_enqueue(p, demote(level));
	}
}

// give up the rest of this turn. the process stays runnable and goes back on
// the queue (one level down) as soon as the scheduler picks the switch up.
void yield(void) {
	struct process *p = myproc();
	struct process *sched_p = mycpu()->scheduler_proc;

	if (!sched_p) {
		panic("yield: no scheduler process to yield to");
	}

	p->state = READY;

	ctx_switch(sched_p);
}
