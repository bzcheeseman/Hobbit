file(GLOB SOURCES *.c)

add_custom_target(OpGen
        COMMAND ${CMAKE_C_COMPILER} -S -emit-llvm ${SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})