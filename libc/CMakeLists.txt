cmake_minimum_required(VERSION 3.16)

# Include directories (matching original Makefile: -Iinclude -I../kernel/kernel/)
set(LIBC_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/kernel/kernel
)

# Architecture-specific settings
set(ARCHDIR arch/${ARCH})

# Common C flags for libc (as a list)
set(LIBC_C_FLAGS -ffreestanding -Wall -Wextra)
set(LIBC_COMPILE_DEFS "__is_libc")

# Kernel libc flags
set(LIBK_C_FLAGS ${LIBC_C_FLAGS})
set(LIBK_COMPILE_DEFS "__is_libk")

# Add architecture-specific flags
# Original Makefile: LIBK_CPPFLAGS includes -I../kernel/arch/i386 (or appropriate arch)
if(ARCH STREQUAL "i386")
    # i386 has minimal arch-specific flags
    set(LIBK_INCLUDE_DIRS ${LIBC_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/kernel/arch/${ARCH})
elseif(ARCH STREQUAL "x86_64")
    list(APPEND LIBK_C_FLAGS
        -fno-stack-protector
        -fno-pic
        -mno-80387
        -mno-mmx
        -mno-sse
        -mno-sse2
        -mno-red-zone
        -mcmodel=kernel
    )
    set(LIBK_INCLUDE_DIRS ${LIBC_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/kernel/arch/${ARCH})
else()
    set(LIBK_INCLUDE_DIRS ${LIBC_INCLUDE_DIRS})
endif()

# Source files
set(FREEOBJS_SOURCES
    stdio/printf.c
    stdio/putchar.c
    stdio/puts.c
    stdlib/abort.c
    string/string.c
)

# Build libk (kernel libc)
add_library(k STATIC ${FREEOBJS_SOURCES})

target_compile_options(k PRIVATE ${LIBK_C_FLAGS})
target_compile_definitions(k PRIVATE ${LIBK_COMPILE_DEFS})
target_include_directories(k PRIVATE ${LIBK_INCLUDE_DIRS})
set_target_properties(k PROPERTIES
    OUTPUT_NAME k
    C_STANDARD 11
)

# Ensure kernel headers are installed before building libk
add_dependencies(k install-kernel-headers)

# Add custom command to install libk.a to sysroot after building
add_custom_command(TARGET k POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}${LIBDIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:k> ${SYSROOT}${LIBDIR}/libk.a
    COMMENT "Installing libk.a to sysroot"
)

# Install headers
install(DIRECTORY include/
    DESTINATION ${INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# Install library
install(TARGETS k
    ARCHIVE DESTINATION ${LIBDIR}
)

# Custom install-headers target for compatibility - runs before anything builds
add_custom_target(install-libc-headers ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SYSROOT}${INCLUDEDIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${SYSROOT}${INCLUDEDIR}
    COMMENT "Installing libc headers"
)

# Ensure libc headers are also installed before building libk
add_dependencies(k install-libc-headers)

# Custom install target for the library
add_custom_target(install-lib
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target install
    DEPENDS k
    COMMENT "Installing libk"
)
