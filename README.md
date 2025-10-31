# Horizon OS

A custom operating system kernel written in C for the i386 architecture.

## Current Features ✅

### Features
- [x] GDT
- [x] Segmentation
- [x] ISR/IQR support
- [x] Virtual Memory - Paging and heap management
- [x] APIC support
- [x] ACPI configuration
- [x] I/O APIC - Interrupt routing and management

### Drivers
- [x] Serial Debugging - qemu debug output via serial console
- [x] Keyboard - Basic input handling
- [x] Timer - System timer functionality
- [x] TTY Driver - Terminal interface

### Misc.
- [x] Custom logging - with colors!

### Libraries & Data Structures
- [x] Standard Library - basic stdio, stdlib, string functions

## Planned features

- [ ] Hush - Horizon Utility Shell (Basic shell)
- [ ] Graphics Driver - VESA VBE framebuffer, or proper support
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

## Building & Testing

```bash

# Clean build artifacts
./clean.sh

# Build the kernel
./iso.sh

# Run in QEMU
./qemu.sh

```

## Architecture

- **Target**: i386 (32-bit x86) and x86_64, with a focus on i386
- **Bootloader**: GRUB
- **Emulation**: QEMU
