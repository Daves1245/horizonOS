.section .text

.global gdt_flush
.type gdt_flush, @function
gdt_flush:
    # load new GDT
    lgdt (%rdi)

    # reload segment registers
    movw $0x10, %ax    # data segment selector
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    # reload CS
    pushq $0x08
    leaq 1f(%rip), %rax
    pushq %rax
    lretq
1:
    ret
.size gdt_flush, . - gdt_flush

.global idt_flush
.type idt_flush, @function
idt_flush:
    lidt (%rdi)
    ret
.size idt_flush, . - idt_flush

.section .note.GNU-stack,"",%progbits
