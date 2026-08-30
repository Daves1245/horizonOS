function(kernel name)
    cmake_parse_arguments(S "" "" "CORE;ASM;DEPENDS;INCLUDES;DEFINES" ${ARGN})

    if (S_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "subsystem ${name}: stray arguments (mispelled arguments?): ${S_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT S_CORE AND NOT S_ASM)
        message(FATAL_ERROR "subsystem ${name}: no sources")
    endif()

    if(NOT TARGET horizon_flags)
        message(FATAL_ERROR "subsystem ${name}: horizon_flags not defined yet")
    endif()

    add_library(${name} OBJECT ${S_CORE} ${S_ASM})

    # horizon_flags carries common flags; 'depends' gives the subsystem
    # the includes of other subsystems (include/kernel and such)
    target_link_libraries(${name} PRIVATE horizon_flags ${S_DEPENDS})

    # Run from the repo root, needs to be personalized to each subsystem
    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/kernel ${S_INCLUDES})
    target_compile_definitions(${name} PRIVATE ${S_DEFINES})

endfunction()
