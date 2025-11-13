#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot

cp sysroot/boot/myos.kernel isodir/myos.kernel
cp limine.cfg isodir/
cp limine.cfg isodir/boot/

LIMINE_DATADIR=$(limine --print-datadir)
cp "$LIMINE_DATADIR/limine-bios.sys" isodir/

# Create a basic ISO
xorriso -as mkisofs \
    -o myos.iso \
    isodir

# Install Limine bootloader to the ISO (makes it bootable as HDD)
limine bios-install myos.iso
