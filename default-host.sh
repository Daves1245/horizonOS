#!/bin/sh
# Check for ARCH environment variable, default to x86_64
ARCH=${ARCH:-x86_64}

case "$ARCH" in
    x86_64)
        if command -v x86_64-linux-gnu-gcc >/dev/null 2>&1; then
            echo x86_64-linux-gnu
        elif command -v x86_64-elf-gcc >/dev/null 2>&1; then
            echo x86_64-elf
        else
            echo "error: x86_64-linux-gnu-gcc or x86_64-elf-gcc not found. install x86_64 cross-compiler or set ARCH=i386" >&2
            exit 1
        fi
        ;;
    i386|i686)
        echo i686-elf
        ;;
    *)
        echo "error: unsupported architecture '$ARCH'. use ARCH=x86_64 or ARCH=i386" >&2
        exit 1
        ;;
esac
