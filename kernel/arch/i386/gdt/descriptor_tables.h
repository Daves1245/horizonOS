// the value of one GDT entry.
// use packed attribute to suggest GCC not add padding
// and align members of the struct.

#include <stdint.h>

struct gdt_entry {
    uint16_t limit_low; // lower 16 bits of limit
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit; // upper 16 bits of all selector limits
    uint32_t base; // address of first gdt_entry struct
} __attribute__((packed));

struct idt_entry {
    uint16_t isr_low;     // lower 16 bits to jmp to when int fires
    uint16_t kernel_cs;   // kernel segment selector
    uint8_t reserved;     // always zero
    uint8_t attributes;   // more flags. see documentation
    uint16_t isr_high;    // upper 16 bits of address to jump to
} __attribute((packed));

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute((packed)) idtr_t;

__attribute__((aligned(0x10))) static struct idt_entry idt[256];

void init_descriptor_tables();
void terminal_initialize();

void isr0();
void isr1();
void isr2();
void isr3();
void isr4();
void isr5();
void isr6();
void isr7();
void isr8();
void isr9();
void isr10();
void isr11();
void isr12();
void isr13();
void isr14();
void isr15();
void isr16();
void isr17();
void isr18();
void isr19();
void isr20();
void isr21();
void isr22();
void isr23();
void isr24();
void isr25();
void isr26();
void isr27();
void isr28();
void isr29();
void isr30();
void isr31();
