; not really useful except for when interacting with legacy machines or embedded systems
; leaving this here in case in the future we'd like to support bare metal on embedded machines
global halt_without_apic
halt_without_apic:
    mov eax, 1
    cpuid
    test edx, (1 << 9) ; 9th bit is apic
    jz halt
    ret
halt:
    cli
    hlt
