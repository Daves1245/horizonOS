# CMake Quick Reference

## Build

```bash
# i386
cmake -S . -B build/i386 -DARCH=i386
cmake --build build/i386
cmake --build build/i386 --target iso

# x86_64
cmake -S . -B build/x86_64 -DARCH=x86_64
cmake --build build/x86_64
cmake --build build/x86_64 --target iso
```

## Clean

```bash
# Clean everything
rm -rf build sysroot

# Clean one architecture
rm -rf build/i386 sysroot
rm -rf build/x86_64 sysroot
```

## Test

```bash
qemu-system-i386 -cdrom build/i386/horizon.iso
qemu-system-x86_64 -cdrom build/x86_64/horizon.iso
```
