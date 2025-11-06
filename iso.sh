#!/bin/sh
set -e

# build production
echo "Building normal version..."
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/myos.kernel isodir/boot/myos.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "myos" {
	multiboot /boot/myos.kernel
}
EOF
grub-mkrescue -o myos.iso isodir

# build dev/debug
echo "Building debug version..."
make -C kernel clean
CPPFLAGS="-DDEBUG" ./build.sh

mkdir -p isodir-debug
mkdir -p isodir-debug/boot
mkdir -p isodir-debug/boot/grub

cp sysroot/boot/myos.kernel isodir-debug/boot/myos.kernel
cat > isodir-debug/boot/grub/grub.cfg << EOF
menuentry "myos-debug" {
	multiboot /boot/myos.kernel
}
EOF
grub-mkrescue -o myos-debug.iso isodir-debug
