# swtch(virt_addr_t *old->sp, virt_addr_t new->sp, phys_addr_t new->pml4_addr)
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

# int clone_parent_base_frame(struct process *child)
# snapshot the caller as a frame swtch() can resume, and have
# fork_copy_stack() copy the stack into the child while the snapshot is still
# live. returns 1 in the parent. the child returns 0 out of this same call
# when it is first swtch()'d into.
.global clone_parent_base_frame
clone_parent_base_frame:
    lea fork_child_resume(%rip), %rax
    push %rax # the child's swtch() rets here, the parent skips it

    # same layout swtch() pushes, so the copy is a frame it can pop
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15

    # child is still in %rdi. 7 pushes on top of the return address leave
    # %rsp 16-byte aligned for the call
    mov %rsp, %rsi
    call clone_stack_rebased

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
    add $8, %rsp

    mov $1, %eax
    ret

fork_child_resume:
    # return value. set to 0 to signify we're the child process
    xor %eax, %eax
    ret # the real return address, back into fork()
