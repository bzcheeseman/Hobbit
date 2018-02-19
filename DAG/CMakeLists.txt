cmake_minimum_required(VERSION 3.5)
project(Hobbit)

include(AddLLVM)

add_subdirectory(../Operator ${CMAKE_BINARY_DIR}/Operator)

file(GLOB SOURCES src/*.cpp)
file(GLOB HEADERS include/*.hpp)

llvm_map_components_to_libnames(llvm_libs all)

add_library(HobbitDAG ${SOURCES} ${HEADERS})
target_link_libraries(HobbitDAG c++ ${llvm_libs} HobbitOperator)
target_include_directories(HobbitDAG PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

#if (BUILD_TESTS)
#    add_gtest(ElementWiseOp HobbitOperator)
#endif (BUILD_TESTS)