cmake_minimum_required(VERSION 3.16)

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
)

# Link flags and libraries
set(KERNEL_LINK_FLAGS -no-pie)
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
        kernel/common/logger.c
        ${ARCHDIR}/gdt/descriptor_tables.c
        ${ARCHDIR}/gdt/gdt.S
        ${ARCHDIR}/interrupts/interrupt.asm
        ${ARCHDIR}/interrupts/isr.c
        ${ARCHDIR}/common/stack_protector.c
        ${ARCHDIR}/common/ctype.c
        ${ARCHDIR}/common/rand.c
        ${ARCHDIR}/memory/paging.c
        ${ARCHDIR}/apic/apic_asm.asm
        ../libc/string/string.c
        ${ARCHDIR}/drivers/shell/hush.c
        ${ARCHDIR}/drivers/shell/builtins/maze.c
        ${ARCHDIR}/panic.c
        kernel/jury/i386/test_paging.c
        kernel/jury/i386/test_vm.c
        ${ARCHDIR}/boot/crtn.S
    )

    set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/${ARCHDIR}/linker.ld)
    set(USE_CRTBEGIN ON)

elseif(ARCH STREQUAL "x86_64")
    # Include uACPI for x86_64 ACPI support
    include(${CMAKE_SOURCE_DIR}/uacpi/uacpi.cmake)

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
        ${ARCHDIR}/panic.c
        ${UACPI_SOURCES}
    )

    # Convert mp3 to raw 16-bit signed LE PCM at 48kHz stereo
    set(AUDIO_MP3 ${CMAKE_SOURCE_DIR}/data/audio.mp3)
    set(AUDIO_RAW ${CMAKE_CURRENT_BINARY_DIR}/audio.raw)
    add_custom_command(
        OUTPUT ${AUDIO_RAW}
        COMMAND ffmpeg -y -i ${AUDIO_MP3} -f s16le -acodec pcm_s16le -ar 48000 -ac 2 ${AUDIO_RAW}
        DEPENDS ${AUDIO_MP3}
        COMMENT "Converting audio.mp3 to raw PCM (16-bit LE, 48kHz, stereo)"
    )

    # Embed raw audio data into a linkable object
    set(AUDIO_OBJ ${CMAKE_CURRENT_BINARY_DIR}/audio.o)
    add_custom_command(
        OUTPUT ${AUDIO_OBJ}
        COMMAND ${TOOLCHAIN_PREFIX}-objcopy -I binary -O elf64-x86-64 -B i386:x86-64
            --rename-section .data=.audio_data,alloc,load,readonly,contents
            ${AUDIO_RAW} ${AUDIO_OBJ}
        DEPENDS ${AUDIO_RAW}
        COMMENT "Embedding audio.raw into audio.o"
    )
    add_custom_target(audio_object DEPENDS ${AUDIO_OBJ})

    set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/${ARCHDIR}/linker.ld)
    set(USE_CRTBEGIN OFF)
endif()

# Common kernel sources
set(KERNEL_COMMON_SOURCES
    ${ARCHDIR}/kernel.c
    kernel/kheap.c
    kernel/drivers/serial.c
    kernel/drivers/timer.c
    kernel/drivers/keyboard/keyboard.c
    kernel/drivers/ac97.c
    kernel/drivers/pci.c
    kernel/apic/rsdp.c
    kernel/apic/apic.c
    kernel/apic/madt.c
    kernel/halt.c
    kernel/mm.c
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

# Link embedded audio data (x86_64 only)
if(ARCH STREQUAL "x86_64")
    add_dependencies(horizon.kernel audio_object)
    target_link_libraries(horizon.kernel ${AUDIO_OBJ})
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

# For i386, verify multiboot compliance
if(ARCH STREQUAL "i386")
    add_custom_command(TARGET horizon.kernel POST_BUILD
        COMMAND grub-file --is-x86-multiboot $<TARGET_FILE:horizon.kernel>
        COMMENT "Verifying multiboot compliance"
    )
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
