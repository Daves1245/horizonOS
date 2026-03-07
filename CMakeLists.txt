cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED ARCH)
    set(ARCH "x86_64" CACHE STRING "target architecture: i386 or x86_64")
endif()

# Validate that build directory matches architecture (if using build/arch structure)
get_filename_component(BUILD_DIR_NAME ${CMAKE_BINARY_DIR} NAME)
if(BUILD_DIR_NAME STREQUAL "i386" OR BUILD_DIR_NAME STREQUAL "x86_64")
    if(NOT BUILD_DIR_NAME STREQUAL ARCH)
        message(FATAL_ERROR "Architecture mismatch! Build directory is '${BUILD_DIR_NAME}' but ARCH is '${ARCH}'. Use: cmake -S . -B build/${ARCH} -DARCH=${ARCH}")
    endif()
endif()

# Toolchain selection matching default-host.sh logic
if(ARCH STREQUAL "i386")
    set(CMAKE_SYSTEM_NAME Generic)
    set(CMAKE_SYSTEM_PROCESSOR i686)
    set(TOOLCHAIN_PREFIX i686-elf)
elseif(ARCH STREQUAL "x86_64")
    include(uacpi/uacpi.cmake)
    set(CMAKE_SYSTEM_NAME Generic)
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
    # Match default-host.sh: try x86_64-linux-gnu-gcc first, then x86_64-elf-gcc
    find_program(X86_64_LINUX_GNU_GCC x86_64-linux-gnu-gcc)
    if(X86_64_LINUX_GNU_GCC)
        set(TOOLCHAIN_PREFIX x86_64-linux-gnu)
    else()
        find_program(X86_64_ELF_GCC x86_64-elf-gcc)
        if(X86_64_ELF_GCC)
            set(TOOLCHAIN_PREFIX x86_64-elf)
        else()
            message(FATAL_ERROR "x86_64-linux-gnu-gcc or x86_64-elf-gcc not found. Install x86_64 cross-compiler or set ARCH=i386")
        endif()
    endif()
else()
    message(FATAL_ERROR "unsupported architecture '${ARCH}'. use ARCH=x86_64 or ARCH=i386")
endif()

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_AR ${TOOLCHAIN_PREFIX}-ar)
set(CMAKE_RANLIB ${TOOLCHAIN_PREFIX}-ranlib)

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_ASM_COMPILER_WORKS 1)

project(Horizon C ASM ASM_NASM)

enable_language(ASM_NASM)

set(SYSROOT ${CMAKE_SOURCE_DIR}/sysroot)
set(PREFIX /usr)
set(EXEC_PREFIX ${PREFIX})
set(BOOTDIR /boot)
set(LIBDIR ${EXEC_PREFIX}/lib)
set(INCLUDEDIR ${PREFIX}/include)

set(CMAKE_INSTALL_PREFIX ${SYSROOT}${PREFIX})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -ffreestanding -Wall -Wextra")

add_compile_options(--sysroot=${SYSROOT})
add_link_options(--sysroot=${SYSROOT})

execute_process(
    COMMAND bash -c "echo ${TOOLCHAIN_PREFIX} | grep -q -- '-elf'"
    RESULT_VARIABLE IS_ELF_TARGET
)
if(IS_ELF_TARGET EQUAL 0)
    add_compile_options(-isystem ${SYSROOT}${INCLUDEDIR})
endif()

if(ARCH STREQUAL "i386")
    set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
    set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} <SOURCE> -o <OBJECT>")
elseif(ARCH STREQUAL "x86_64")
    set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
    set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} <SOURCE> -o <OBJECT>")
endif()

add_subdirectory(libc)
add_subdirectory(kernel)

add_custom_target(install-all
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}${LIBDIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}${BOOTDIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/libc/libk.a ${SYSROOT}${LIBDIR}/libk.a
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/kernel/horizon.kernel ${SYSROOT}${BOOTDIR}/horizon.kernel
    DEPENDS k horizon.kernel install-kernel-headers install-libc-headers
    COMMENT "installing all components to sysroot"
)

if(ARCH STREQUAL "i386")
    add_custom_target(iso
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/isodir
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/isodir/boot/grub
        COMMAND ${CMAKE_COMMAND} -E copy
            ${SYSROOT}${BOOTDIR}/horizon.kernel
            ${CMAKE_BINARY_DIR}/isodir/boot/horizon.kernel
        COMMAND ${CMAKE_COMMAND}
            -DGRUB_CFG_PATH=${CMAKE_BINARY_DIR}/isodir/boot/grub/grub.cfg
            -P ${CMAKE_SOURCE_DIR}/cmake/create_grub_cfg.cmake
        COMMAND grub-mkrescue -o ${CMAKE_BINARY_DIR}/horizon.iso ${CMAKE_BINARY_DIR}/isodir
        DEPENDS install-all
        COMMENT "creating bootable i386 iso"
    )
elseif(ARCH STREQUAL "x86_64")
    # Find limine
    find_program(LIMINE_BINARY NAMES limine PATHS /usr/local/bin /usr/bin)
    if(NOT LIMINE_BINARY)
        message(WARNING "limine not found, x86_64 ISO target may not work")
    endif()

    add_custom_target(iso
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/isodir
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/isodir
        COMMAND ${CMAKE_COMMAND} -E copy
            ${SYSROOT}${BOOTDIR}/horizon.kernel
            ${CMAKE_BINARY_DIR}/isodir/horizon.kernel
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/limine.conf
            ${CMAKE_BINARY_DIR}/isodir/
        COMMAND ${CMAKE_COMMAND}
            -DLIMINE_BINARY=${LIMINE_BINARY}
            -DISODIR=${CMAKE_BINARY_DIR}/isodir
            -P ${CMAKE_SOURCE_DIR}/cmake/copy_limine_files.cmake
        COMMAND xorriso -as mkisofs
            -b limine-bios.sys
            -no-emul-boot
            -boot-load-size 4
            -boot-info-table
            -o ${CMAKE_BINARY_DIR}/horizon.iso
            ${CMAKE_BINARY_DIR}/isodir/
        COMMAND ${LIMINE_BINARY} bios-install ${CMAKE_BINARY_DIR}/horizon.iso
        DEPENDS install-all
        COMMENT "creating bootable x86_64 iso"
    )
endif()

# Build target that respects the proper order: headers -> libc -> kernel
# Mimics the original build.sh workflow
add_custom_target(build-all
    DEPENDS k horizon.kernel install-kernel-headers install-libc-headers
    COMMENT "Building everything in correct order"
)

# Default ALL target just builds kernel and libs, not ISO
add_custom_target(build_default ALL
    DEPENDS k horizon.kernel
)

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${SYSROOT}
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/isodir
    COMMAND ${CMAKE_COMMAND} -E rm -f
        ${CMAKE_BINARY_DIR}/horizon.iso
        ${CMAKE_BINARY_DIR}/horizon.iso
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMENT "cleaning"
)
