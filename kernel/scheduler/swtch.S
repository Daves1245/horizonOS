# swtch(virt_addr_t *old->rsp, virt_addr_t new->rsp, virt_addr_t new->cr3)
# arguments in asm are rdi, rsi, rdx, ...
.global swtch
swtch:
    # save old stack
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15

    mov %rsp, (%rdi)
    mov %rsi, %rsp # load new stack
    mov %rdx, %cr3

    # TODO(multicore) insert TLB flush here

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
    ret # the instruction where it happens
