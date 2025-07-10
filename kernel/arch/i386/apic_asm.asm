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
