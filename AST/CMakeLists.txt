cmake_minimum_required(VERSION 3.5)
project(Hobbit)

include(AddLLVM)

file(GLOB SOURCES src/*.cpp)
file(GLOB HEADERS include/*.hpp)

llvm_map_components_to_libnames(llvm_libs all)

add_library(HobbitAST SHARED ${SOURCES} ${HEADERS})
target_link_libraries(HobbitAST c++ ${llvm_libs})
target_include_directories(HobbitAST PUBLIC ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (BUILD_TESTS)
#    add_gtest(Core HobbitCore)
endif (BUILD_TESTS)