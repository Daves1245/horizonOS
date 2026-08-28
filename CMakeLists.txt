cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# Toolchain selection
if(ARCH STREQUAL "i386")
    set(CMAKE_SYSTEM_NAME Generic)
    set(CMAKE_SYSTEM_PROCESSOR i686)
    set(TOOLCHAIN_PREFIX i686-elf)
elseif(ARCH STREQUAL "x86_64")
    include(uacpi/uacpi.cmake)
    set(CMAKE_SYSTEM_NAME Generic)
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
    # try x86_64-linux-gnu-gcc first, then x86_64-elf-gcc

    # detect OS and set proper gcc
    if(WIN32)
        find_program(WIN32_X86_64_ELF_GCC x86_64-elf-gcc)
        if(WIN32_X86_64_ELF_GCC)
            set(TOOLCHAIN_PREFIX x86_64-elf)
        else()
            message(FATAL_ERROR "x86_64-elf-gcc not found. try: pacman -S mingw-w64-cross-gcc, or download from winlibs.com")
        endif()
    elseif(APPLE)
        find_program(MACOS_X86_64_ELF_GCC x86_64-elf-gcc)
        if(MACOS_X86_64_ELF_GCC)
            set(TOOLCHAIN_PREFIX x86_64-elf)
        else()
            message(FATAL_ERROR "x86_64-elf-gcc not found. try: brew install x86_64-elf-gcc")
        endif()
    else()
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
    endif()
else()
    message(FATAL_ERROR "unsupported architecture '${ARCH}'. use ARCH=x86_64 or ARCH=i386")
endif()

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_AR ${TOOLCHAIN_PREFIX}-ar)
set(CMAKE_RANLIB ${TOOLCHAIN_PREFIX}-ranlib)

# Check for readelf
find_program(READELF_BINARY ${TOOLCHAIN_PREFIX}-readelf)
if(NOT READELF_BINARY)
    if(WIN32)
        message(FATAL_ERROR "${TOOLCHAIN_PREFIX}-readelf not found. try: pacman -S mingw-w64-cross-binutils")
    elseif(APPLE)
        message(FATAL_ERROR "${TOOLCHAIN_PREFIX}-readelf not found. try: brew install x86_64-elf-binutils")
    else()
        message(FATAL_ERROR "${TOOLCHAIN_PREFIX}-readelf not found. try: sudo apt install binutils (or equivalent)")
    endif()
endif()

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_ASM_COMPILER_WORKS 1)

# Check for NASM
find_program(NASM_BINARY nasm)
if(NOT NASM_BINARY)
    if(WIN32)
        message(FATAL_ERROR "nasm not found. try: pacman -S nasm, or download from nasm.us")
    elseif(APPLE)
        message(FATAL_ERROR "nasm not found. try: brew install nasm")
    else()
        message(FATAL_ERROR "nasm not found. try: sudo apt install nasm  (or equivalent)")
    endif()
endif()
set(CMAKE_ASM_NASM_COMPILER ${NASM_BINARY})

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
# ---------------------------------------------------------------------------
# kernel
# ---------------------------------------------------------------------------

# Architecture directory
set(ARCHDIR arch/${ARCH})

# Kernel compile flags (as a list)
set(KERNEL_C_FLAGS -O0 -g -v -ffreestanding -Wall -Wextra -fstack-protector)
set(KERNEL_COMPILE_DEFS "__is_kernel")

# Include directories
set(KERNEL_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/${ARCHDIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/arch
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers
    # last so an arch-local drivers/ header
    # (arch/x86_64/drivers/ps2k.h) still wins
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link flags and libraries
set(KERNEL_LINK_FLAGS -no-pie -z noexecstack)
set(KERNEL_LIBS -nostdlib -lk -lgcc)

# Architecture-specific configuration
if(ARCH STREQUAL "i386")
    set(KERNEL_ARCH_C_FLAGS -mno-sse -mno-red-zone -mno-mmx -mno-sse2 -mno-3dnow -mno-avx -msoft-float -mno-80387)
    set(KERNEL_ARCH_COMPILE_DEFS "__i386__")
    set(KERNEL_ARCH_INCLUDE_DIRS ${ARCHDIR}/common)

    set(KERNEL_ARCH_SOURCES
        ${ARCHDIR}/boot/crti.S
        ${ARCHDIR}/boot/boot.S
        ${ARCHDIR}/drivers/tty.c
        ${ARCHDIR}/common/logger.c
        ${ARCHDIR}/gdt/descriptor_tables.c
        ${ARCHDIR}/gdt/gdt.S
        ${ARCHDIR}/interrupts/interrupt.asm
        ${ARCHDIR}/interrupts/isr.c
        ${ARCHDIR}/common/stack_protector.c
        ${ARCHDIR}/common/ctype.c
        ${ARCHDIR}/common/rand.c
        ${ARCHDIR}/memory/paging.c
        ${ARCHDIR}/apic/apic_asm.asm
        libc/string/string.c
        ${ARCHDIR}/drivers/shell/hush.c
        ${ARCHDIR}/drivers/shell/builtins/maze.c
        ${ARCHDIR}/drivers/shell/builtins/peek.c
        ${ARCHDIR}/drivers/graphics.c
        ${ARCHDIR}/panic.c
        ${ARCHDIR}/jury/test_paging.c
        ${ARCHDIR}/jury/test_vm.c
        ${ARCHDIR}/boot/crtn.S
    )

    # convert audio to 16-bit 48kHz stereo raw PCM for AC97
    set(AUDIO_PONG_WALL_SRC ${CMAKE_SOURCE_DIR}/audio/wall.ogg)
    set(AUDIO_PONG_WALL_RAW ${CMAKE_CURRENT_BINARY_DIR}/wall.raw)
    set(AUDIO_PONG_PADDLE_SRC ${CMAKE_SOURCE_DIR}/audio/paddle.ogg)
    set(AUDIO_PONG_PADDLE_RAW ${CMAKE_CURRENT_BINARY_DIR}/paddle.raw)
    set(AUDIO_PONG_SCORE_SRC ${CMAKE_SOURCE_DIR}/audio/score.ogg)
    set(AUDIO_PONG_SCORE_RAW ${CMAKE_CURRENT_BINARY_DIR}/score.raw)

    add_custom_command(
        OUTPUT ${AUDIO_PONG_WALL_RAW}
        COMMAND ffmpeg -y -i ${AUDIO_PONG_WALL_SRC} -f s16le -acodec pcm_s16le -ar 48000 -ac 2 ${AUDIO_PONG_WALL_RAW}
        DEPENDS ${AUDIO_PONG_WALL_SRC}
        COMMENT "Converting wall.ogg to raw PCM"
    )
    add_custom_command(
        OUTPUT ${AUDIO_PONG_PADDLE_RAW}
        COMMAND ffmpeg -y -i ${AUDIO_PONG_PADDLE_SRC} -f s16le -acodec pcm_s16le -ar 48000 -ac 2 ${AUDIO_PONG_PADDLE_RAW}
        DEPENDS ${AUDIO_PONG_PADDLE_SRC}
        COMMENT "Converting paddle.ogg to raw PCM"
    )
    add_custom_command(
        OUTPUT ${AUDIO_PONG_SCORE_RAW}
        COMMAND ffmpeg -y -i ${AUDIO_PONG_SCORE_SRC} -f s16le -acodec pcm_s16le -ar 48000 -ac 2 ${AUDIO_PONG_SCORE_RAW}
        DEPENDS ${AUDIO_PONG_SCORE_SRC}
        COMMENT "Converting score.ogg to raw PCM"
    )

    set(I386_WALL_OBJ ${CMAKE_CURRENT_BINARY_DIR}/wall.o)
    add_custom_command(
        OUTPUT ${I386_WALL_OBJ}
        COMMAND ${TOOLCHAIN_PREFIX}-objcopy -I binary -O elf32-i386 -B i386
            --rename-section .data=.sound_wall,alloc,load,readonly,contents
            ${AUDIO_PONG_WALL_RAW} ${I386_WALL_OBJ}
        DEPENDS ${AUDIO_PONG_WALL_RAW}
        COMMENT "Embedding wall.raw (i386)"
    )
    add_custom_target(i386_wall_obj DEPENDS ${I386_WALL_OBJ})

    set(I386_PADDLE_OBJ ${CMAKE_CURRENT_BINARY_DIR}/paddle.o)
    add_custom_command(
        OUTPUT ${I386_PADDLE_OBJ}
        COMMAND ${TOOLCHAIN_PREFIX}-objcopy -I binary -O elf32-i386 -B i386
            --rename-section .data=.sound_paddle,alloc,load,readonly,contents
            ${AUDIO_PONG_PADDLE_RAW} ${I386_PADDLE_OBJ}
        DEPENDS ${AUDIO_PONG_PADDLE_RAW}
        COMMENT "Embedding paddle.raw (i386)"
    )
    add_custom_target(i386_paddle_obj DEPENDS ${I386_PADDLE_OBJ})

    set(I386_SCORE_OBJ ${CMAKE_CURRENT_BINARY_DIR}/score.o)
    add_custom_command(
        OUTPUT ${I386_SCORE_OBJ}
        COMMAND ${TOOLCHAIN_PREFIX}-objcopy -I binary -O elf32-i386 -B i386
            --rename-section .data=.sound_score,alloc,load,readonly,contents
            ${AUDIO_PONG_SCORE_RAW} ${I386_SCORE_OBJ}
        DEPENDS ${AUDIO_PONG_SCORE_RAW}
        COMMENT "Embedding score.raw (i386)"
    )
    add_custom_target(i386_score_obj DEPENDS ${I386_SCORE_OBJ})

    set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/${ARCHDIR}/linker.ld)
    set(USE_CRTBEGIN ON)

elseif(ARCH STREQUAL "x86_64")
    # uACPI sources/includes come from the include() in the toolchain block above
    set(KERNEL_ARCH_C_FLAGS -no-pie -mcmodel=kernel -fno-pic -fstack-protector -mstack-protector-guard=global -mno-red-zone -mno-sse -mno-mmx -mno-sse2 -mno-avx -msoft-float)
    set(KERNEL_ARCH_COMPILE_DEFS "__x86_64__" "UACPI_OVERRIDE_CONFIG")
    set(KERNEL_ARCH_INCLUDE_DIRS ${ARCHDIR}/include ${ARCHDIR}/uacpi ${UACPI_INCLUDES})

    set(KERNEL_ARCH_SOURCES
        ${ARCHDIR}/boot/boot.S
        ${ARCHDIR}/drivers/tty.c
        ${ARCHDIR}/common/logger.c
        ${ARCHDIR}/interrupts/descriptor_tables.c
        ${ARCHDIR}/interrupts/gdt.S
        ${ARCHDIR}/interrupts/isr.c
        ${ARCHDIR}/interrupts/stack_protector.c
        ${ARCHDIR}/interrupts/common.c
        ${ARCHDIR}/interrupts/apic.c
        ${ARCHDIR}/memory/paging.c
        ${ARCHDIR}/asm/interrupt.S
        ${ARCHDIR}/asm/apic_asm.S
        ${ARCHDIR}/uacpi/kernel_api.c
        ${ARCHDIR}/uacpi/uacpi_init.c
        ${ARCHDIR}/acpi/acpi_bus.c
        ${ARCHDIR}/drivers/ps2k.c
        ${ARCHDIR}/drivers/graphics.c
        ${ARCHDIR}/panic.c
        ${ARCHDIR}/include/log.c
        ${ARCHDIR}/include/shell.c
        ${ARCHDIR}/include/keyboard.c
        ${ARCHDIR}/scheduler/lock.c
        ${ARCHDIR}/scheduler/process.c
        ${ARCHDIR}/scheduler/scheduler.c
        ${ARCHDIR}/scheduler/swtch.S
        ${UACPI_SOURCES}
    )

    # convert audio to 16-bit 48kHz stereo raw PCM for AC97
    # -f s16le: raw signed 16-bit little-endian PCM
    # -acodec pcm_s16le: 16-bit LE PCM codec
    # -ar 48000: 48kHz sample rate (AC97 standard)
    # -ac 2: stereo

    set(AUDIO_RICK_SRC ${CMAKE_SOURCE_DIR}/audio/rick.mp3)
    set(AUDIO_RICK_RAW ${CMAKE_CURRENT_BINARY_DIR}/rick.raw)
    set(AUDIO_PONG_WALL_SRC ${CMAKE_SOURCE_DIR}/audio/wall.ogg)
    set(AUDIO_PONG_WALL_RAW ${CMAKE_CURRENT_BINARY_DIR}/wall.raw)
    set(AUDIO_PONG_PADDLE_SRC ${CMAKE_SOURCE_DIR}/audio/paddle.ogg)
    set(AUDIO_PONG_PADDLE_RAW ${CMAKE_CURRENT_BINARY_DIR}/paddle.raw)
    set(AUDIO_PONG_SCORE_SRC ${CMAKE_SOURCE_DIR}/audio/score.ogg)
    set(AUDIO_PONG_SCORE_RAW ${CMAKE_CURRENT_BINARY_DIR}/score.raw)

    add_custom_command(
        OUTPUT ${AUDIO_RICK_RAW}
        COMMAND ffmpeg -y -i ${AUDIO_RICK_SRC} -f s16le -acodec pcm_s16le -ar 48000 -ac 2 ${AUDIO_RICK_RAW}
        DEPENDS ${AUDIO_RICK_SRC}
        COMMENT "Converting rick.mp3 to raw PCM"
    )
    add_custom_command(
        OUTPUT ${AUDIO_PONG_WALL_RAW}
        COMMAND ffmpeg -y -i ${AUDIO_PONG_WALL_SRC} -f s16le -acodec pcm_s16le -ar 48000 -ac 2 ${AUDIO_PONG_WALL_RAW}
        DEPENDS ${AUDIO_PONG_WALL_SRC}
        COMMENT "Converting wall.ogg to raw PCM"
    )
    add_custom_command(
        OUTPUT ${AUDIO_PONG_PADDLE_RAW}
        COMMAND ffmpeg -y -i ${AUDIO_PONG_PADDLE_SRC} -f s16le -acodec pcm_s16le -ar 48000 -ac 2 ${AUDIO_PONG_PADDLE_RAW}
        DEPENDS ${AUDIO_PONG_PADDLE_SRC}
        COMMENT "Converting paddle.ogg to raw PCM"
    )
    add_custom_command(
        OUTPUT ${AUDIO_PONG_SCORE_RAW}
        COMMAND ffmpeg -y -i ${AUDIO_PONG_SCORE_SRC} -f s16le -acodec pcm_s16le -ar 48000 -ac 2 ${AUDIO_PONG_SCORE_RAW}
        DEPENDS ${AUDIO_PONG_SCORE_SRC}
        COMMENT "Converting score.ogg to raw PCM"
    )

    # Embed raw audio data into linkable objects
    set(RICK_OBJ ${CMAKE_CURRENT_BINARY_DIR}/rick.o)
    add_custom_command(
        OUTPUT ${RICK_OBJ}
        COMMAND ${TOOLCHAIN_PREFIX}-objcopy -I binary -O elf64-x86-64 -B i386:x86-64
            --rename-section .data=.sound_rick,alloc,load,readonly,contents
            ${AUDIO_RICK_RAW} ${RICK_OBJ}
        DEPENDS ${AUDIO_RICK_RAW}
        COMMENT "Embedding rick.raw"
    )
    add_custom_target(rick_obj DEPENDS ${RICK_OBJ})

    set(WALL_OBJ ${CMAKE_CURRENT_BINARY_DIR}/wall.o)
    add_custom_command(
        OUTPUT ${WALL_OBJ}
        COMMAND ${TOOLCHAIN_PREFIX}-objcopy -I binary -O elf64-x86-64 -B i386:x86-64
            --rename-section .data=.sound_wall,alloc,load,readonly,contents
            ${AUDIO_PONG_WALL_RAW} ${WALL_OBJ}
        DEPENDS ${AUDIO_PONG_WALL_RAW}
        COMMENT "Embedding wall.raw"
    )
    add_custom_target(wall_obj DEPENDS ${WALL_OBJ})

    set(PADDLE_OBJ ${CMAKE_CURRENT_BINARY_DIR}/paddle.o)
    add_custom_command(
        OUTPUT ${PADDLE_OBJ}
        COMMAND ${TOOLCHAIN_PREFIX}-objcopy -I binary -O elf64-x86-64 -B i386:x86-64
            --rename-section .data=.sound_paddle,alloc,load,readonly,contents
            ${AUDIO_PONG_PADDLE_RAW} ${PADDLE_OBJ}
        DEPENDS ${AUDIO_PONG_PADDLE_RAW}
        COMMENT "Embedding paddle.raw"
    )
    add_custom_target(paddle_obj DEPENDS ${PADDLE_OBJ})

    set(SCORE_OBJ ${CMAKE_CURRENT_BINARY_DIR}/score.o)
    add_custom_command(
        OUTPUT ${SCORE_OBJ}
        COMMAND ${TOOLCHAIN_PREFIX}-objcopy -I binary -O elf64-x86-64 -B i386:x86-64
            --rename-section .data=.sound_score,alloc,load,readonly,contents
            ${AUDIO_PONG_SCORE_RAW} ${SCORE_OBJ}
        DEPENDS ${AUDIO_PONG_SCORE_RAW}
        COMMENT "Embedding score.raw"
    )
    add_custom_target(score_obj DEPENDS ${SCORE_OBJ})

    set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/${ARCHDIR}/linker.ld)
    set(USE_CRTBEGIN OFF)
endif()

# Common kernel sources
set(KERNEL_COMMON_SOURCES
    ${ARCHDIR}/kernel.c
    kernel/kheap.c
    drivers/timer.c
    drivers/keyboard/keyboard.c
    drivers/ac97.c
    drivers/pci.c
    drivers/graphics.c
    drivers/font.c
    drivers/console.c
    kernel/fonts/fontlibrary.c
    kernel/apic/rsdp.c
    kernel/apic/apic.c
    kernel/apic/madt.c
    kernel/halt.c
    kernel/mm.c
    kernel/games/pong.c
    include/serial.c
)

set(SUBSYSTEMS
    arch
    drivers
    kernel
    fs
)

# All kernel sources
set(KERNEL_SOURCES
    ${KERNEL_ARCH_SOURCES}
    ${KERNEL_COMMON_SOURCES}
)

# Create kernel executable
add_executable(horizon.kernel ${KERNEL_SOURCES})

# Ensure headers and libk are ready before building kernel
add_dependencies(horizon.kernel install-kernel-headers install-libc-headers k)

# Link embedded audio data
if(ARCH STREQUAL "x86_64")
    add_dependencies(horizon.kernel rick_obj wall_obj paddle_obj score_obj)
    target_link_libraries(horizon.kernel ${RICK_OBJ} ${WALL_OBJ} ${PADDLE_OBJ} ${SCORE_OBJ})
elseif(ARCH STREQUAL "i386")
    add_dependencies(horizon.kernel i386_wall_obj i386_paddle_obj i386_score_obj)
    target_link_libraries(horizon.kernel ${I386_WALL_OBJ} ${I386_PADDLE_OBJ} ${I386_SCORE_OBJ})
endif()

# Set compile options
target_compile_options(horizon.kernel PRIVATE
    ${KERNEL_C_FLAGS}
    ${KERNEL_ARCH_C_FLAGS}
)

target_compile_definitions(horizon.kernel PRIVATE
    ${KERNEL_COMPILE_DEFS}
    ${KERNEL_ARCH_COMPILE_DEFS}
)

target_include_directories(horizon.kernel PRIVATE
    ${KERNEL_INCLUDE_DIRS}
    ${KERNEL_ARCH_INCLUDE_DIRS}
)

# includes for libc
target_include_directories(k PRIVATE
    ${KERNEL_INCLUDE_DIRS}
    ${KERNEL_ARCH_INCLUDE_DIRS}
)

# Get crtbegin and crtend if needed for i386
if(USE_CRTBEGIN)
    # Get paths to crtbegin.o and crtend.o from GCC
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} ${KERNEL_C_FLAGS} ${KERNEL_ARCH_C_FLAGS} -print-file-name=crtbegin.o
        OUTPUT_VARIABLE CRTBEGIN_OBJ
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} ${KERNEL_C_FLAGS} ${KERNEL_ARCH_C_FLAGS} -print-file-name=crtend.o
        OUTPUT_VARIABLE CRTEND_OBJ
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Copy crtbegin.o and crtend.o to build directory
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/crtbegin.o
        COMMAND ${CMAKE_COMMAND} -E copy ${CRTBEGIN_OBJ} ${CMAKE_CURRENT_BINARY_DIR}/crtbegin.o
        COMMENT "Copying crtbegin.o"
    )
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/crtend.o
        COMMAND ${CMAKE_COMMAND} -E copy ${CRTEND_OBJ} ${CMAKE_CURRENT_BINARY_DIR}/crtend.o
        COMMENT "Copying crtend.o"
    )

    add_custom_target(crt_objects
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/crtbegin.o ${CMAKE_CURRENT_BINARY_DIR}/crtend.o
    )
    add_dependencies(horizon.kernel crt_objects)
endif()

# Set link options
target_link_options(horizon.kernel PRIVATE
    -T ${LINKER_SCRIPT}
    ${KERNEL_LINK_FLAGS}
    -L${SYSROOT}${LIBDIR}
)

# Link libraries
target_link_libraries(horizon.kernel
    ${KERNEL_LIBS}
)

# Kernel links against libk.a in sysroot (installed by build-all target)
# Do NOT add dependency on k here - it causes circular dependencies

# Ensure linker script is a dependency
set_target_properties(horizon.kernel PROPERTIES
    LINK_DEPENDS ${LINKER_SCRIPT}
)

# Post-build commands: generate disassembly, symbols, and elf info
add_custom_command(TARGET horizon.kernel POST_BUILD
    COMMAND ${TOOLCHAIN_PREFIX}-objdump -d $<TARGET_FILE:horizon.kernel> > ${CMAKE_CURRENT_BINARY_DIR}/horizon.kernel.dis
    COMMAND ${TOOLCHAIN_PREFIX}-objdump -t $<TARGET_FILE:horizon.kernel> > ${CMAKE_CURRENT_BINARY_DIR}/horizon.kernel.sym
    COMMAND ${READELF_BINARY} -a $<TARGET_FILE:horizon.kernel> > ${CMAKE_CURRENT_BINARY_DIR}/horizon.kernel.elf
    COMMENT "Generating kernel disassembly, symbols, and ELF info"
)

# For i386, verify multiboot compliance (skipped if grub-file is not installed)
if(ARCH STREQUAL "i386")
    find_program(GRUB_FILE grub-file)
    if(GRUB_FILE)
        add_custom_command(TARGET horizon.kernel POST_BUILD
            COMMAND ${GRUB_FILE} --is-x86-multiboot $<TARGET_FILE:horizon.kernel>
            COMMENT "Verifying multiboot compliance"
        )
    endif()
endif()

# Install kernel
install(TARGETS horizon.kernel
    RUNTIME DESTINATION ${BOOTDIR}
)

# Custom install-headers target - runs before anything builds
add_custom_target(install-kernel-headers ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}${INCLUDEDIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SYSROOT}${INCLUDEDIR}
    COMMENT "Installing kernel headers"
)
# Make sure this runs early
set_target_properties(install-kernel-headers PROPERTIES
    EXCLUDE_FROM_ALL FALSE
)

add_custom_target(install-all
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}${LIBDIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}${BOOTDIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/libc/libk.a ${SYSROOT}${LIBDIR}/libk.a
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/horizon.kernel ${SYSROOT}${BOOTDIR}/horizon.kernel
    DEPENDS k horizon.kernel install-kernel-headers install-libc-headers
    COMMENT "installing all components to sysroot"
)

if(ARCH STREQUAL "i386")
    find_program(GRUB_MKRESCUE NAMES i686-elf-grub-mkrescue grub-mkrescue)
    if(NOT GRUB_MKRESCUE)
        message(FATAL_ERROR "grub-mkrescue not found. Install grub: sudo apt install grub-pc-bin (on linux). For mac, try: brew install i686-elf-grub (mac)")
    endif()
    add_custom_target(iso
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/isodir
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/isodir/boot/grub
        COMMAND ${CMAKE_COMMAND} -E copy
            ${SYSROOT}${BOOTDIR}/horizon.kernel
            ${CMAKE_BINARY_DIR}/isodir/boot/horizon.kernel
        COMMAND ${CMAKE_COMMAND}
            -DGRUB_CFG_PATH=${CMAKE_BINARY_DIR}/isodir/boot/grub/grub.cfg
            -P ${CMAKE_SOURCE_DIR}/cmake/create_grub_cfg.cmake
        COMMAND ${GRUB_MKRESCUE} -o ${CMAKE_BINARY_DIR}/horizon.iso ${CMAKE_BINARY_DIR}/isodir
        DEPENDS install-all
        COMMENT "creating bootable i386 iso"
    )
elseif(ARCH STREQUAL "x86_64")
    # Find limine
    find_program(LIMINE_BINARY NAMES limine PATHS /usr/local/bin /usr/bin)
    if(NOT LIMINE_BINARY)
        message(WARNING "limine not found, x86_64 ISO target may not work")
    endif()

    find_program(XORRISO_BINARY xorriso)
    if(NOT XORRISO_BINARY)
        if(WIN32)
            message(FATAL_ERROR "xorriso not found. try: pacman -S xorriso")
        elseif(APPLE)
            message(FATAL_ERROR "xorriso not found. try: brew install xorriso")
        else()
            message(FATAL_ERROR "xorriso not found. try: sudo apt install xorriso")
        endif()
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
        COMMAND ${XORRISO_BINARY} -as mkisofs
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
