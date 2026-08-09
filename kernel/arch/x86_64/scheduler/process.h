#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <x86_64/memory/paging.h>
#include <mm.h>

#define NUM_PROCESSES 10
#define NUM_CPUS 2

// TODO(userspace)
// track TSS and load new SS and ESP on context switch for switching
// to a higher privilege level

struct context {
  uint16_t eip;
  uint16_t rsi;
  /* general registers */

  uint16_t rbp;
  uint16_t rbx;
  uint16_t r12;
  uint16_t r13;
  uint16_t r14;
  uint16_t r15;

  /* segment registers */
  uint16_t cs; // code
  uint16_t ss; // stack
  uint16_t ds; // data
  uint16_t es; // extra
  uint16_t fs; // general/thread (TLS)
  uint16_t gs; // general/cpu

  uint8_t cr3; // addrof top level page structure

  // ffu, mmx, see registers etc. (not currently used)
};

enum process_state {
  READY,
  RUNNING,
  SLEEPING,
  ZOMBIE
};

struct cpu {
  int cpuid;
  int intena;
  int noff;
};

struct cpu cpus[NUM_CPUS];

struct process {
  uint8_t pid;
  char name[32];

  struct process *parent;

  virt_addr_t kstack;
  enum process_state state;
  struct page_table_t pagetable;
  struct context context;
};

struct process processes[NUM_PROCESSES];

struct process *fork(struct process *p);

void sched();

#endif
