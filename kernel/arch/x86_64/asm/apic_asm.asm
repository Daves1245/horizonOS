global read_msr_asm
global write_msr_asm

read_msr_asm:
    ; RDI contains MSR number
    mov ecx, edi
    rdmsr
    ; Combine EDX:EAX into RAX
    shl rdx, 32
    or rax, rdx
    ret

write_msr_asm:
    ; RDI contains MSR number, RSI contains value
    mov ecx, edi
    mov rax, rsi
    mov rdx, rsi
    shr rdx, 32
    wrmsr
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
