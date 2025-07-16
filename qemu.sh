#!/bin/sh

# determine architecture
ARCH=${ARCH:-x86_64}
DEFAULT_HOST=$(./default-host.sh 2>/dev/null || echo "i686-elf")
HOSTARCH=$(./target-triplet-to-arch.sh "$DEFAULT_HOST" 2>/dev/null || echo "i386")

# select appropriate QEMU
if [ "$HOSTARCH" = "x86_64" ]; then
    QEMU_SYSTEM="qemu-system-x86_64"
else
    QEMU_SYSTEM="qemu-system-i386"
fi

# either debug mode (connect to gdb) or normal mode
if [ "$1" = "debug" ]; then
    $QEMU_SYSTEM -drive file=myos.iso,format=raw,index=0,media=disk -S -s -vga std -machine pc,smm=off -d cpu,int,guest_errors,exec -trace events=events -D qemu.log
else
    $QEMU_SYSTEM -drive file=myos.iso,format=raw,index=0,media=disk -vga std -machine pc,smm=off -d cpu,int,guest_errors,unimp -trace events=events -D qemu.log
fi
