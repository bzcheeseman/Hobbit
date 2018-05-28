cmake_minimum_required(VERSION 3.5)
project(Hobbit)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG -fno-rtti")

find_package(glog REQUIRED)
include(AddLLVM)

option(BUILD_TESTS "Whether or not to build the tests" ON)

set(GTEST_CFG ${CMAKE_MODULE_PATH})
set(GTEST_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
include(AddGTest)

enable_testing()
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} -V)

llvm_map_components_to_libnames(llvm_libs all)

file(GLOB CODEGEN ${CMAKE_CURRENT_SOURCE_DIR}/include/codegen/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen/*.cpp)
file(GLOB GRAPH ${CMAKE_CURRENT_SOURCE_DIR}/include/graph/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/src/graph/*.cpp)
file(GLOB OPS ${CMAKE_CURRENT_SOURCE_DIR}/include/ops/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/src/ops/*.cpp)
file(GLOB UTILS ${CMAKE_CURRENT_SOURCE_DIR}/include/utils/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/*.cpp)
file(GLOB COMPILE ${CMAKE_CURRENT_SOURCE_DIR}/include/compile/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/src/compile/*.cpp)
add_library(Hobbit SHARED ${CODEGEN} ${GRAPH} ${OPS} ${UTILS} ${COMPILE})
target_link_libraries(Hobbit PUBLIC ${llvm_libs} Polly isl c++ glog::glog)
target_include_directories(Hobbit PUBLIC ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(passes)

add_gtest(AST Hobbit)

include(AddClangFormat)
