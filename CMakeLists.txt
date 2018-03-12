cmake_minimum_required(VERSION 3.5)
project(Hobbit)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

option(BUILD_TESTS "Whether or not to build the tests" ON)

if (BUILD_TESTS)
    set(GTEST_CFG ${CMAKE_MODULE_PATH})
    set(GTEST_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
    include(AddGTest)

    enable_testing()

    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} -V)

    function(add_gtest test_name lib)
        add_executable(Test${test_name} EXCLUDE_FROM_ALL ${CMAKE_CURRENT_SOURCE_DIR}/test/Test${test_name}.cpp)
        target_link_libraries(Test${test_name} gtest_main ${lib})
        add_test(NAME Test${test_name} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/Test${test_name})
        add_dependencies(check Test${test_name})
    endfunction()

endif (BUILD_TESTS)

add_subdirectory(Buffer)
add_subdirectory(Module)

file(GLOB_RECURSE
        ALL_CXX_SOURCE_FILES
        **/*.[chi]pp **/*.[chi]
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
