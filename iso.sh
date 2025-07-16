#!/bin/sh
set -e
. ./build.sh

rm -rf isodir
mkdir -p isodir/

cp sysroot/boot/myos.kernel isodir/myos.kernel
cp limine.conf isodir/

LIMINE_DATADIR=$(limine --print-datadir)
cp "$LIMINE_DATADIR/limine-bios.sys" isodir/

# create bootable ISO - specify full path from ISO root
xorriso -as mkisofs \
    -b limine-bios.sys \
    -no-emul-boot \
    -boot-load-size 4 \
    -boot-info-table \
    -o myos.iso \
    isodir/

limine bios-install myos.iso
