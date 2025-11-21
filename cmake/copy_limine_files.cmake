# Script to copy limine files for x86_64 ISO
execute_process(
    COMMAND ${LIMINE_BINARY} --print-datadir
    OUTPUT_VARIABLE LIMINE_DATADIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE RESULT
)

if(NOT RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to get limine datadir")
endif()

file(COPY "${LIMINE_DATADIR}/limine-bios.sys"
     DESTINATION "${ISODIR}")
