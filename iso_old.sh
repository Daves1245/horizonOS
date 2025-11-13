#!/bin/sh
set -e
. ./build.sh

rm -rf isodir
mkdir -p isodir/

cp sysroot/boot/myos.kernel isodir/myos.kernel
cp limine.cfg isodir/

LIMINE_DATADIR=$(limine --print-datadir)
cp "$LIMINE_DATADIR/limine-bios.sys" isodir/

# Create a basic ISO
xorriso -as mkisofs \
    -b limine-bios.sys \
    -boot-load-size 4 \
    -o myos.iso \
    isodir

# Install Limine bootloader to the ISO (makes it bootable as HDD)
limine bios-install myos.iso
