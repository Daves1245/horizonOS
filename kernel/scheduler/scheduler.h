#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <kernel/scheduler/process.h>

#define NUM_PRIORITY_LEVELS 3

void init_scheduler(void);
void sched_enqueue(struct process *p, int level);
void scheduler(void);
void yield(void);
void ctx_switch(struct process *p);

#endif
