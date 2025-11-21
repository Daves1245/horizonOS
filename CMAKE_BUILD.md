# CMake Build System for Horizon OS

This project has been migrated from nested Makefiles to CMake for better cross-platform support and maintainability.

## Prerequisites

- CMake 3.16 or higher
- Cross-compiler toolchain (i686-elf-gcc or x86_64-elf-gcc)
- NASM assembler
- For i386: GRUB tools (grub-mkrescue, grub-file)
- For x86_64: Limine bootloader tools, xorriso

## Quick Start

This project uses **CMake Presets** for easy architecture switching. Each architecture has its own build directory, so you can switch between them without reconfiguring!

### Building for i386 (default)

```bash
# Configure and build in one command
cmake --preset i386 && cmake --build --preset i386

# Or separately:
cmake --preset i386              # Configure
cmake --build --preset i386      # Build

# The ISO is automatically created: horizon-i386.iso
```

### Building for x86_64

```bash
# Configure and build in one command
cmake --preset x86_64 && cmake --build --preset x86_64

# Or separately:
cmake --preset x86_64            # Configure
cmake --build --preset x86_64    # Build

# The ISO is automatically created: horizon-x86_64.iso
```

### Switching Between Architectures

Since each architecture has its own build directory, you can freely switch:

```bash
# Build i386
cmake --preset i386 && cmake --build --preset i386

# Now build x86_64 without reconfiguring i386
cmake --preset x86_64 && cmake --build --preset x86_64

# Go back to i386 and rebuild
cmake --build --preset i386
```

### Alternative: Manual Configuration (without presets)

If you prefer explicit control:

```bash
# i386
cmake -B build/i386 -DARCH=i386
cmake --build build/i386

# x86_64
cmake -B build/x86_64 -DARCH=x86_64
cmake --build build/x86_64
```

## Cleaning the Build

To completely clean everything:

```bash
# Remove all build artifacts, sysroot, and ISOs
rm -rf build/ sysroot/ isodir/ *.iso
```

To clean just one architecture:

```bash
# Clean i386 only
rm -rf build/i386

# Clean x86_64 only
rm -rf build/x86_64
```

To use the built-in clean target (keeps configuration):

```bash
cmake --build build/i386 --target clean-all
# or
cmake --build build/x86_64 --target clean-all
```

## Available Targets

### Default target (ALL)
```bash
cmake --build --preset i386    # or x86_64
```
Builds the kernel, libc, installs to sysroot, and creates the ISO.

### Individual targets

Using build directory directly:
```bash
# Build only libk (kernel libc)
cmake --build build/i386 --target k

# Build only the kernel
cmake --build build/i386 --target horizon.kernel

# Install to sysroot
cmake --build build/i386 --target install-all

# Create ISO only (requires install-all first)
cmake --build build/i386 --target iso

# Clean everything including sysroot
cmake --build build/i386 --target clean-all
```

Replace `build/i386` with `build/x86_64` for x86_64 builds.

## Project Structure

```
horizon/
├── CMakeLists.txt          # Root build configuration
├── CMakePresets.json       # Architecture presets
├── libc/
│   └── CMakeLists.txt      # libc/libk build configuration
├── kernel/
│   └── CMakeLists.txt      # Kernel build configuration
├── build/                  # Build output directory
│   ├── i386/               # i386 build files
│   └── x86_64/             # x86_64 build files
└── sysroot/                # Installation prefix (auto-generated)
    ├── usr/
    │   ├── include/        # Installed headers
    │   └── lib/            # Installed libraries (libk.a)
    └── boot/
        └── horizon.kernel  # Installed kernel
```

## Architecture-Specific Details

### i386
- Uses GRUB as bootloader
- Multiboot compliant
- Includes crtbegin.o and crtend.o
- Output: `horizon-i386.iso`

### x86_64
- Uses Limine as bootloader
- Higher-half kernel mapping
- Red-zone disabled, -mcmodel=kernel
- Output: `horizon-x86_64.iso`

## Compiler Flags

Both architectures use:
- `-ffreestanding`: Freestanding environment
- `-Wall -Wextra`: All warnings
- `-O2 -g`: Optimized with debug info
- `-fstack-protector`: Stack protection

Architecture-specific flags are defined in each kernel/CMakeLists.txt.

## Comparison with Old Makefile System

| Old Command | New CMake Command |
|------------|-------------------|
| `make i386` | `cmake --preset i386 && cmake --build --preset i386` |
| `make x86_64` | `cmake --preset x86_64 && cmake --build --preset x86_64` |
| `make clean` | `cmake --build --preset <arch> --target clean-all` |
| `./build.sh` | Integrated into CMake presets |

## Troubleshooting

### "Cross-compiler not found"
Make sure your cross-compiler (i686-elf-gcc or x86_64-elf-gcc) is in your PATH.

### "NASM not found"
Install NASM: `sudo apt install nasm` (or equivalent for your distro)

### Build in wrong directory
Always run cmake from the project root. Use presets or `-B` to specify build directory.

### Reconfigure
If you change CMakeLists.txt files or need to reconfigure:
```bash
# Using presets (recommended)
rm -rf build/i386
cmake --preset i386

# Or manually
rm -rf build/i386
cmake -B build/i386 -DARCH=i386
```
