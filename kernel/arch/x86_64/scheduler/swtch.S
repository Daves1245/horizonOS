# swtch(uint64_t old_rsp, uint64_t new_rsp, uint64_t new_cr3)
.global swtch
swtch:
    # save old stack
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15

    mov %cr3, %rax
    push %rax

    mov %rsp, (%rdi)
    mov %rsi, %rsp # load new stack

    pop %rax
    mov %rax, %cr3
    # TODO(multicore) insert TLB flush here

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
    ret # the instruction where it happens
