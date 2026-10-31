# swtch(virt_addr_t *old->sp, virt_addr_t new->sp, phys_addr_t new->pml4_addr)
# i386 cdecl passes everything on the stack, so read the three arguments out
# before the pushes below move %esp out from under them.
.global swtch
swtch:
    mov 4(%esp), %eax  # &old->context.sp
    mov 8(%esp), %edx  # new->context.sp
    mov 12(%esp), %ecx # new->pml4_addr

    # save old stack. eax/ecx/edx are caller-saved, so the four below are
    # the whole callee-saved set the i386 System V ABI asks us to carry
    push %ebp
    push %ebx
    push %esi
    push %edi

    mov %esp, (%eax)
    mov %edx, %esp # load new stack
    mov %ecx, %cr3

    # TODO(multicore) insert TLB flush here

    pop %edi
    pop %esi
    pop %ebx
    pop %ebp
    ret # the instruction where it happens

# int copy_stack(struct process *child)
# snapshot the parent's frame such that swtch() can resume, and have
# fork_copy_stack() copy the stack into the child while the snapshot is still
# live. returns 1 in the parent. the child returns 0 out of this same call
# when it is first swtch()'d into.
.global clone_parent_base_frame
clone_parent_base_frame:
    mov 4(%esp), %eax # child

    push $fork_child_resume # the child's swtch() rets here, the parent skips it

    # same layout swtch() pushes, so the copy is a frame it can pop
    push %ebp
    push %ebx
    push %esi
    push %edi

    mov %esp, %ecx
    push %ecx # frame
    push %eax # child
    call clone_stack_rebased
    add $8, %esp

    pop %edi
    pop %esi
    pop %ebx
    pop %ebp
    add $4, %esp

    mov $1, %eax
    ret

fork_child_resume:
    # return value, we're the child process so return 0
    xor %eax, %eax
    ret # the real return address, back into fork()
