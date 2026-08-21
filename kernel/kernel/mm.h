#ifndef MM_H
#define MM_H

#include <stdint.h>
#include <dsa/list.h>

/* Virtual Memory Regions */
enum vm_type {
    STACK,
    HEAP,
};

#define FL_VM_PROTECTED (1 << 0)
#define FL_VM_READ (1 << 1)
#define FL_VM_WRITE (1 << 2)
#define LF_VM_EXECUTE (1 << 3)

#ifdef __x86_64__
typedef uint64_t virt_addr_t;
typedef uint64_t phys_addr_t;
extern uint64_t hhdm_offset;
extern uint64_t kernel_phys_base;
extern uint64_t kernel_virt_base;
#else
typedef uint32_t virt_addr_t;
typedef uint32_t phys_addr_t;
#endif

struct vm_region {
    uint64_t flags;
    enum vm_type type;
    virt_addr_t start;
    virt_addr_t end;

    struct list_head node; // a process' vm_region linked list
};

phys_addr_t virt_to_phys(virt_addr_t addr);
virt_addr_t phys_to_virt(phys_addr_t addr);

#endif
