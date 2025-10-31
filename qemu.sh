#!/bin/sh

if [ "$1" = "gdb" ]; then
    # gdb with stub
    qemu-system-i386 -cdrom myos.iso -S -s -kernel isodir/boot/myos.kernel \
        -serial file:serial.log
elif [ "$1" = "debug" ]; then
    # debug mode with monitor and interrupt tracing
    qemu-system-i386 -cdrom myos.iso -kernel isodir/boot/myos.kernel \
        -monitor stdio \
        -d int,cpu_reset,guest_errors \
        -D debug.log \
        -no-reboot \
        -no-shutdown \
        -serial file:serial.log
elif [ "$1" = "serial" ]; then
    # serial console mode - all kernel output to serial.log
    qemu-system-i386 -cdrom myos.iso -kernel isodir/boot/myos.kernel \
        -nographic \
        -serial mon:stdio \
        -no-reboot \
        -no-shutdown
else
    # run normally
    qemu-system-i386 -cdrom myos.iso -kernel isodir/boot/myos.kernel \
        -serial stdio
fi
