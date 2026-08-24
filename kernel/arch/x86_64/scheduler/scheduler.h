#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <x86_64/scheduler/process.h>

#define NUM_PRIORITY_LEVELS 3

void init_scheduler();
void sched_enqueue(struct process *p, int level);
void scheduler();
void yield();
void ctx_switch(struct process *p);

#endif
