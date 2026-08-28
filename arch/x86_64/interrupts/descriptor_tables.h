#ifndef DESCRIPTOR_TABLES_H
#define DESCRIPTOR_TABLES_H

#include <stdint.h>

// GDT entry structure for x86_64
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

// GDT pointer structure
struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

// IDT entry structure for x86_64 (64-bit)
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

// IDT pointer structure
struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

// Function prototypes
void init_descriptor_tables(void);
extern void gdt_flush(uint64_t);
extern void idt_flush(uint64_t);

// ISR stub table (will be defined later)
extern void* isr_stub_table[];

#endif