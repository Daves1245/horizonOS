#!/bin/sh
set -e

# build production
echo "Building normal version..."
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/myos.kernel isodir/boot/horizon.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "horizon" {
	multiboot /boot/horizon.kernel
}
EOF
grub-mkrescue -o horizon.iso isodir

# build dev/debug
echo "Building debug version..."

mkdir -p isodir-debug
mkdir -p isodir-debug/boot
mkdir -p isodir-debug/boot/grub

cp sysroot/boot/myos.kernel isodir-debug/boot/horizon.kernel
cat > isodir-debug/boot/grub/grub.cfg << EOF
menuentry "horizon-debug" {
	multiboot /boot/horizon.kernel
}
EOF
grub-mkrescue -o horizon-debug.iso isodir-debug
