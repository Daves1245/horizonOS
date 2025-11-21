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
- [x] Keyboard driver
- [x] Timer driver
- [x] Makeshift shell
- [x] Logging

### x86_64
- [x] Limine support
- [x] Logging
- [x] APIC support

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

see [CMAKE_BUILD.md](CMAKE_BUILD.md) for more details.

---

*[vibes](https://www.poetryfoundation.org/poems/50457/i-saw-a-man-pursuing-the-horizon)*
