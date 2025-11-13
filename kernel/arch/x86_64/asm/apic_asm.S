# x86_64 APIC Assembly Functions
# TODO: Implement proper APIC assembly functions

.section .text

# Placeholder APIC function
.global apic_dummy
.type apic_dummy, @function
apic_dummy:
    # TODO: APIC assembly functions
    ret
.size apic_dummy, . - apic_dummy

.section .note.GNU-stack,"",%progbits