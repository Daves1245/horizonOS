# x86_64 Boot Assembly - Minimal setup for Limine
# Limine will call kernel_main directly, so this is mostly for any
# additional initialization if needed

.section .text

# Halt and catch fire function for x86_64
.global hcf
.type hcf, @function
hcf:
    cli
1:  hlt
    jmp 1b
.size hcf, . - hcf

.section .note.GNU-stack,"",%progbits