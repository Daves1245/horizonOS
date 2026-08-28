# x86_64 Boot Assembly - Minimal setup for Limine
# Limine will call kernel_main directly, so this is mostly for any
# additional initialization if needed

.section .note.GNU-stack,"",%progbits