cmake_minimum_required(VERSION 3.5)
project(Hobbit)

include(AddLLVM)

file(GLOB SOURCES src/*.cpp)
file(GLOB HEADERS include/*.hpp)

llvm_map_components_to_libnames(llvm_libs all)

add_library(HobbitModule ${SOURCES} ${HEADERS})
target_link_libraries(HobbitModule c++ ${llvm_libs})
target_include_directories(HobbitModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

#if (BUILD_TESTS)
#    add_gtest(ElementWiseOp HobbitOperator)
#endif (BUILD_TESTS)