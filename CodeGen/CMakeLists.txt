cmake_minimum_required(VERSION 3.5)
project(Hobbit)

include(AddLLVM)

file(GLOB SOURCES src/*.cpp)
file(GLOB HEADERS include/*.hpp)

llvm_map_components_to_libnames(llvm_libs all)

add_library(HobbitCodeGen SHARED ${SOURCES} ${HEADERS})
target_link_libraries(HobbitCodeGen c++ ${llvm_libs} glog::glog HobbitAST)
target_include_directories(HobbitCodeGen PUBLIC ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (BUILD_TESTS)
#    add_gtest(AST HobbitAST)
endif (BUILD_TESTS)