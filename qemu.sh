#!/bin/sh

# Detect which architecture to use (default to i386 if both exist)
if [ -f build/i386/kernel/horizon.kernel ]; then
    ARCH="i386"
    ISO_FILE="horizon.iso"
    KERNEL_PATH="build/i386/kernel/horizon.kernel"
    QEMU_BIN="qemu-system-i386"
    USE_CDROM=1
elif [ -f build/x86_64/horizon.iso ]; then
    ARCH="x86_64"
    ISO_FILE="horizon.iso"
    QEMU_BIN="qemu-system-x86_64"
    USE_CDROM=0
else
    echo "No ISO found. Build with 'make i386' or 'make x86_64' first."
    exit 1
fi

echo "Running $ARCH (ISO: $ISO_FILE)"

if [ "$USE_CDROM" = "1" ]; then
    # i386: Direct kernel boot with ISO as CD-ROM
    if [ "$1" = "debug" ]; then
        $QEMU_BIN -cdrom $ISO_FILE -kernel $KERNEL_PATH \
            -vga std \
            -monitor stdio \
            -d int,cpu_reset,guest_errors \
            -D debug.log \
            -no-reboot \
            -no-shutdown \
            -serial file:serial.log
    elif [ "$1" = "serial" ]; then
        $QEMU_BIN -cdrom $ISO_FILE -kernel $KERNEL_PATH \
            -nographic \
            -serial mon:stdio \
            -no-reboot \
            -no-shutdown
    else
        $QEMU_BIN -cdrom $ISO_FILE -kernel $KERNEL_PATH \
            -vga std \
            -serial stdio
    fi
else
    # x86_64: Boot from disk via Limine
    if [ "$1" = "debug" ]; then
        $QEMU_BIN -drive file=$ISO_FILE,format=raw,index=0,media=disk \
            -vga std \
            -machine pc,smm=off \
            -d cpu,int,guest_errors,exec \
            -D qemu.log \
            -no-reboot \
            -no-shutdown \
            -serial file:serial.log
    else
        $QEMU_BIN -drive file=$ISO_FILE,format=raw,index=0,media=disk \
            -vga std \
            -machine pc,smm=off \
            -serial stdio
    fi
fi
