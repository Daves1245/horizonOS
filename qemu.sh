#!/bin/sh

if [ "$1" = "debug" ]; then
    # If any argument is passed, enable GDB stub
    qemu-system-i386 -cdrom myos.iso -S -s -kernel isodir/boot/myos.kernel
else
    # Run normally without GDB
    qemu-system-i386 -cdrom myos.iso -kernel isodir/boot/myos.kernel
fi
