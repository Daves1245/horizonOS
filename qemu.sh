#!/bin/sh

if [ "$1" = "debug" ]; then
    # debug mode with GDB stub
    qemu-system-i386 -cdrom myos.iso -S -s -kernel isodir/boot/myos.kernel \
        -monitor stdio \
        -d int,cpu_reset \
        -no-reboot \
        -no-shutdown \
        -serial file:serial.log
elif [ "$1" = "console" ]; then
    # console mode for direct byte inspection
    qemu-system-i386 -cdrom myos.iso -kernel isodir/boot/myos.kernel \
        -monitor stdio \
        -d int,cpu_reset,exec,in_asm \
        -D qemu.log \
        -no-reboot \
        -no-shutdown \
        -serial file:serial.log
else
    # run normally
    qemu-system-i386 -cdrom myos.iso -kernel isodir/boot/myos.kernel \
        -monitor stdio \
        -serial file:serial.log
fi
