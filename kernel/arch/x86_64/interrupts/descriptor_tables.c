#include <stdint.h>
#include <string.h>
#include "descriptor_tables.h"

static struct gdt_entry gdt_entries[5];
static struct gdt_ptr gdt_ptr;

static struct idt_entry idt_entries[256];
static struct idt_ptr idt_ptr;

static void init_gdt(void);
static void init_idt(void);
static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
static void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags);

void init_descriptor_tables(void) {
    init_gdt();
    init_idt();
}

static void init_gdt(void) {
    gdt_ptr.limit = (sizeof(struct gdt_entry) * 5) - 1;
    gdt_ptr.base = (uint64_t) &gdt_entries;

    // 64 bit
    // null segment
    gdt_set_gate(0, 0, 0, 0, 0);
    // kernel code segment
    gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0xAF);
    // kernel data segment
    gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0xCF);
    // user code segment
    gdt_set_gate(3, 0, 0xFFFFF, 0xFA, 0xAF);
    // user data segment
    gdt_set_gate(4, 0, 0xFFFFF, 0xF2, 0xCF);
    gdt_flush((uint64_t) &gdt_ptr);
}

static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access = access;
}

static void init_idt(void) {
    idt_ptr.limit = sizeof(struct idt_entry) * 256 - 1;
    idt_ptr.base = (uint64_t) &idt_entries;
    memset(&idt_entries, 0, sizeof(struct idt_entry) * 256);
    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, (uint64_t) isr_stub_table[i], 0x08, 0x8E);
    }
    idt_flush((uint64_t) &idt_ptr);
}

static void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].offset_low = base & 0xFFFF;
    idt_entries[num].offset_mid = (base >> 16) & 0xFFFF;
    idt_entries[num].offset_high = (base >> 32) & 0xFFFFFFFF;

    idt_entries[num].selector = sel;
    idt_entries[num].ist = 0;
    idt_entries[num].type_attr = flags;
    idt_entries[num].zero = 0;
}
