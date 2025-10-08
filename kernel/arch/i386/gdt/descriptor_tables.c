//
// descriptor_tables.c - Initialises the GDT and IDT, and defines the
//                       default ISR and IRQ handler.
//                       Based on code from Bran's kernel development tutorials.
//                       Rewritten for JamesM's kernel development tutorials.
//

#include <string.h>
#include <stdint.h>

#include "common/common.h"
#include "descriptor_tables.h"

// I/O port functions for PIC programming
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Lets us access our ASM functions from our C code.
extern void gdt_flush(u32int);
extern void idt_flush(u32int);

// Internal function prototypes.
static void init_gdt();
static void init_idt();
static void gdt_set_gate(s32int,u32int,u32int,u8int,u8int);

struct gdt_entry gdt_entries[5];
struct gdt_ptr   gdt_ptr;
struct idt_entry idt_entries[256];

// idt
__attribute__((aligned(0x10))) static struct idt_entry idt[256];
idtr_t idtr;

// Initialisation routine - zeroes all the interrupt service routines,
// initialises the GDT and IDT.
void init_descriptor_tables() {
    // Initialise the global descriptor table.
    init_gdt();
    // Initialise the interrupt descriptor table.
    init_idt();
}

static void init_gdt() {
    gdt_ptr.limit = (sizeof(struct gdt_entry) * 5) - 1;
    gdt_ptr.base  = (u32int)&gdt_entries;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment

    gdt_flush((u32int)&gdt_ptr);
}

// Set the value of one GDT entry.
static void gdt_set_gate(s32int num, u32int base, u32int limit, u8int access, u8int gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

static void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags) {
    struct idt_entry *descriptor = &idt[vector];

    descriptor->isr_low = (uint32_t) isr & 0xFFFF;
    descriptor->kernel_cs = 0x08; // can be whatever offset your kernel code selector is in your GDT
    descriptor->attributes = flags;
    descriptor->isr_high = (uint32_t) isr >> 16;
    descriptor->reserved = 0;
}

void idt_flush(u32int);

// TODO move to a better spot
#define IDT_MAX_DESCRIPTORS 256

static int vectors[IDT_MAX_DESCRIPTORS];
extern void *isr_stub_table[];

// Remap the PIC to use IRQs 32-47 instead of 8-23
static void remap_pic() {
    // Save masks
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);

    // Start initialization sequence
    outb(0x20, 0x11);  // ICW1: Initialize PIC1
    outb(0xA0, 0x11);  // ICW1: Initialize PIC2
    
    // ICW2: Set vector offsets
    outb(0x21, 32);    // PIC1 offset: IRQ 0-7 -> interrupts 32-39
    outb(0xA1, 40);    // PIC2 offset: IRQ 8-15 -> interrupts 40-47
    
    // ICW3: Set up cascading
    outb(0x21, 0x04);  // PIC1: IRQ2 connected to slave
    outb(0xA1, 0x02);  // PIC2: Cascade identity
    
    // ICW4: Set mode
    outb(0x21, 0x01);  // 8086 mode
    outb(0xA1, 0x01);  // 8086 mode
    
    // Enable timer IRQ 0, keyboard IRQ 1, and cascade IRQ 2
    outb(0x21, 0xF8);  // 11111000 - Enable IRQ 0 (timer), IRQ 1 (keyboard), IRQ 2 (cascade)
    outb(0xA1, 0xFF);  // Mask all IRQs on PIC2 for now
}

static void init_idt() {
    idtr.base = (uintptr_t) &idt[0];
    idtr.limit = sizeof(struct idt_entry) * IDT_MAX_DESCRIPTORS - 1;

    // Set up exception handlers (0-31)
    for (uint8_t vector = 0; vector < 32; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = 1;
    }

    // set up irq handlers (32-34 for now, just for keyboard driver)
    for (uint8_t vector = 32; vector <= 34; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = 1;
    }

    // Remap PIC before enabling interrupts
    remap_pic();

    __asm__ volatile ("lidt %0" : : "m"(idtr)); // load the new IDT
    __asm__ volatile ("sti"); // enable flag
}
