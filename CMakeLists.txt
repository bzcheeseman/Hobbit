cmake_minimum_required(VERSION 3.5)
project(Hobbit)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

find_package(glog REQUIRED)
include(AddLLVM)

option(BUILD_TESTS "Whether or not to build the tests" ON)

set(GTEST_CFG ${CMAKE_MODULE_PATH})
set(GTEST_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
include(AddGTest)

enable_testing()
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} -V)

llvm_map_components_to_libnames(llvm_libs all)

file(GLOB CODEGEN ${CMAKE_CURRENT_SOURCE_DIR}/include/codegen/*.hpp)
file(GLOB GRAPH ${CMAKE_CURRENT_SOURCE_DIR}/include/graph/*.hpp)
file(GLOB OPS ${CMAKE_CURRENT_SOURCE_DIR}/include/ops/*.hpp)
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c*)
add_library(Hobbit SHARED ${CODEGEN} ${GRAPH} ${OPS} ${SOURCES})
target_link_libraries(Hobbit PUBLIC c++ ${llvm_libs} glog::glog)
target_include_directories(Hobbit PUBLIC ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(registry)
add_subdirectory(KernelFusion)

add_gtest(DataStorage Hobbit)
add_gtest(AST Hobbit HobbitOpeltwise_add)

include(AddClangFormat)
