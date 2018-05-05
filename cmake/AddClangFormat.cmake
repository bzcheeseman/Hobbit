file(GLOB_RECURSE
        ALL_CXX_SOURCE_FILES
        include/**/*.[chi]pp include/**/*.[chi]
        src/*.[chi]pp src/*.[chi]
        registry/**/*.[chi]pp registry/**/*.[chi]
        registry/*.[chi]pp registry/*.[chi]
        test/*.[chi]pp test/*.[chi]
        KernelFusion/**/*.[chi]pp KernelFusion/**/*.[chi]
        )

find_program(CLANG_FORMAT "clang-format")
if(CLANG_FORMAT)
    add_custom_target(
            clang-format
            COMMAND ${CLANG_FORMAT}
            -i
            -style=file
            ${ALL_CXX_SOURCE_FILES}

    )
endif()