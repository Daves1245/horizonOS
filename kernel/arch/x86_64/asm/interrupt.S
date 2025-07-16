# x86_64 Interrupt Assembly Stubs
# TODO: Implement proper interrupt stubs

.section .text

# Placeholder interrupt stub
.global interrupt_stub
.type interrupt_stub, @function
interrupt_stub:
    # TODO: Save registers and call C handler
    iretq
.size interrupt_stub, . - interrupt_stub

.section .note.GNU-stack,"",%progbits