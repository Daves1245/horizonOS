#   term 1: ./qemu.sh gdb
#   term 2: x86_64-elf-gdb build/x86_64/horizon.kernel

set confirm off
set architecture i386:x86-64
set disassembly-flavor intel

target remote :1234

hbreak kernel_main
continue
