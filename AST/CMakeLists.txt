cmake_minimum_required(VERSION 3.5)
project(Hobbit)

file(GLOB SOURCES src/*.c*)
file(GLOB HEADERS include/*.h*)

llvm_map_components_to_libnames(llvm_libs all)

add_library(HobbitAST SHARED ${SOURCES} ${HEADERS})
target_link_libraries(HobbitAST c++ ${llvm_libs} glog::glog)
target_include_directories(HobbitAST PUBLIC ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (BUILD_TESTS)
    add_gtest(AST HobbitAST)
endif (BUILD_TESTS)