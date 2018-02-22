cmake_minimum_required(VERSION 3.5)
project(Hobbit)

include(AddLLVM)

file(GLOB SOURCES src/*.cpp)
file(GLOB HEADERS include/*.hpp)

llvm_map_components_to_libnames(llvm_libs all)

add_library(HobbitOperation SHARED ${SOURCES} ${HEADERS})
target_link_libraries(HobbitOperation c++ ${llvm_libs} HobbitBuffer)
target_include_directories(HobbitOperation PUBLIC ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (BUILD_TESTS)
    add_gtest(CompositeOp HobbitOperation)
    add_gtest(ElementWiseOp HobbitOperation)
    add_gtest(ReductionOp HobbitOperation)
endif (BUILD_TESTS)