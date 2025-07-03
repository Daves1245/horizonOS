%macro isr_err_stub 1
global isr%+%1
isr%+%1:
    cli
    push %1                    ; Push interrupt number
    jmp isr_common_stub
%endmacro

%macro isr_no_err_stub 1
global isr%+%1
isr%+%1:
    cli
    push 0                     ; Push dummy error code
    push %1                    ; Push interrupt number
    jmp isr_common_stub
%endmacro

extern isr_handler
global isr_common_stub
isr_common_stub:
    pusha                      ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

    xor eax, eax               ; Clear eax. Not necessary, but upper bits are displayed in struct register
    mov ax, ds                 ; Lower 16-bits of eax = ds (zero extension not needed in assembly, but to properly fill out our struct value, we zero extend so that the full 32 bit value doesn't have gargbage upper 16 bits)
    push eax                   ; Save the data segment descriptor (push ds)

    mov ax, 0x10               ; Load the kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; -- Ensure stack alignment for System V ABI (16-byte for function calls) --

    mov eax, esp               ; Move stack pointer into eax
    and esp, -16               ; Align stack to 16-byte boundary (0xFFFFFFF0)

    call isr_handler
    add esp, 4                 ; Clean up the pushed argument

    ; -- Restore stack --
    pop esp                    ; Unaligned stack pointer value

    pop eax                    ; Reload the original data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                       ; Pops edi,esi,ebp,esp,ebx,edx,ecx,eax
    add esp, 8                 ; Cleans up the pushed error code and pushed ISR number
    iret                       ; Pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP
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

global isr_stub_table
isr_stub_table:
%assign i 0
%rep    32
    dd isr%+i
%assign i i+1
%endrep
