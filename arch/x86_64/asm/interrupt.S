# x86_64 Interrupt Assembly Stubs (GAS syntax)
# Equivalent to interrupt.asm (NASM) but for the x86_64 GCC toolchain

.section .text

.extern isr_handler

# Common stub - saves state, calls C handler, restores state
.global isr_common_stub
.type isr_common_stub, @function
isr_common_stub:
    # Save all general purpose registers
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rbp
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # Save data segment
    xorq %rax, %rax
    movw %ds, %ax
    pushq %rax

    # Load kernel data segment
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Call C handler with pointer to stack frame (System V ABI: first arg in rdi)
    movq %rsp, %rdi
    call isr_handler

    # Restore data segment
    popq %rax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Restore all general purpose registers
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rbp
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax

    # Remove error code and interrupt number
    addq $16, %rsp

    iretq
.size isr_common_stub, . - isr_common_stub

# Macro for ISR with no error code (pushes dummy 0)
.macro isr_no_err_stub num
.global isr\num
.type isr\num, @function
isr\num:
    cli
    pushq $0
    pushq $\num
    jmp isr_common_stub
.size isr\num, . - isr\num
.endm

# Macro for ISR with hardware error code (already on stack)
.macro isr_err_stub num
.global isr\num
.type isr\num, @function
isr\num:
    cli
    pushq $\num
    jmp isr_common_stub
.size isr\num, . - isr\num
.endm

# CPU exceptions (0-31)
isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_no_err_stub 21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_err_stub    30
isr_no_err_stub 31

# IRQ handlers (32-47)
isr_no_err_stub 32
isr_no_err_stub 33
isr_no_err_stub 34
isr_no_err_stub 35
isr_no_err_stub 36
isr_no_err_stub 37
isr_no_err_stub 38
isr_no_err_stub 39
isr_no_err_stub 40
isr_no_err_stub 41
isr_no_err_stub 42
isr_no_err_stub 43
isr_no_err_stub 44
isr_no_err_stub 45
isr_no_err_stub 46
isr_no_err_stub 47

# ISR stub table - array of pointers to each handler
.section .data
.global isr_stub_table
isr_stub_table:
.quad isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7
.quad isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15
.quad isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
.quad isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
.quad isr32, isr33, isr34, isr35, isr36, isr37, isr38, isr39
.quad isr40, isr41, isr42, isr43, isr44, isr45, isr46, isr47

.section .note.GNU-stack,"",%progbits
