# HorizonOS

A toy kernel written in C targeting both x86 and x86_64

## Currently Implemented Features

### i386
- [x] GDT
- [x] Segmentation
- [x] ISR/IQR support
- [x] Paging
- [x] Heap allocated memory
- [x] APIC support
- [x] ACPI configuration
- [x] PS/2 keyboard driver
- [x] Timer driver
- [x] Makeshift shell
- [x] Logging

### x86_64
- [x] Limine support
- [x] Logging
- [x] APIC support
- [x] uACPI full mode
- [x] PS/2 keyboard driver
- [x] AC97 audio driver 

### Custom libc implementation (bootstrapped)
- [x] Standard Library - basic stdio, stdlib, string functions

## Timeline

### i386
- [ ] Graphics Driver - VESA VBE framebuffer, or proper support

### x86_64
- [ ] Hush - Horizon Utility Shell (Basic shell)

### Both
- [ ] File System - Basic filesystem implementation (FAT32 or custom)
- [ ] System Calls - User/kernel mode interface
- [ ] Text editor - vim worsened
- [ ] User Mode - Ring 3 execution environment
- [ ] Multiprocessing w/ ACPI infrastructure
- [ ] Process Management - Task switching and scheduling
- [ ] Scheduler
- [ ] VFS - Virtual File System abstraction
- [ ] IPC Mechanisms - pipes, shared memory, message queues
- [ ] Network stack - TCP/IP implementation
- [ ] Ethernet driver
- [ ] USB driver
- [ ] ELF execution
- [ ] GUI Framework - basic windowing system

### Ubuntu/Debian
```bash
sudo apt install cmake nasm xorriso qemu-system-x86 grub-pc-bin grub-common binutils

# x86_64 cross-compiler
sudo apt install gcc-x86-64-linux-gnu binutils-x86-64-linux-gnu
# or build x86_64-elf-gcc from source: https://wiki.osdev.org/GCC_Cross-Compiler

# i386 cross-compiler
sudo apt install gcc-i686-linux-gnu binutils-i686-linux-gnu

# Limine (x86_64 only)
# See: https://github.com/limine-bootloader/limine/blob/v10.x/INSTALL.md
```

### Arch Linux
```bash
sudo pacman -S cmake nasm libisoburn qemu-system-x86 grub gcc binutils

# Cross-compilers and Limine (AUR)
yay -S i686-elf-gcc x86_64-elf-gcc limine
```

### macOS
```bash
brew install cmake nasm xorriso qemu

# Cross-compilers
brew install x86_64-elf-gcc x86_64-elf-binutils  # x86_64
brew install i686-elf-gcc i686-elf-binutils        # i386

# Limine (x86_64 only)
# See: https://github.com/limine-bootloader/limine/blob/v10.x/INSTALL.md
```

### Windows (MSYS2)

Untested, YMMV

```bash
pacman -S cmake nasm mingw-w64-cross-gcc mingw-w64-cross-binutils xorriso

# Limine (x86_64 only)
# See: https://github.com/limine-bootloader/limine/blob/v10.x/INSTALL.md
```


## Build & Run
```bash
# i386
cmake --preset i386
cmake --build --preset i386

# x86_64
cmake --preset x86_64
cmake --build --preset x86_64

# Run with qemu
./qemu.sh # will automatically detect architecture
```

See [CMAKE_BUILD.md](CMAKE_BUILD.md) for more details.

---

*[vibes](https://www.poetryfoundation.org/poems/50457/i-saw-a-man-pursuing-the-horizon)*
