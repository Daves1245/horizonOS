#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <x86_64/scheduler/process.h>

void scheduler();
void yield();
void ctx_switch(struct process *p);

#endif
