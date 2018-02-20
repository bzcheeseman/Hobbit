cmake_minimum_required(VERSION 3.5)
project(Hobbit)

include(AddLLVM)

file(GLOB SOURCES src/*.cpp)
file(GLOB HEADERS include/*.hpp)

llvm_map_components_to_libnames(llvm_libs all)

add_library(HobbitOperator SHARED ${SOURCES} ${HEADERS})
target_link_libraries(HobbitOperator c++ ${llvm_libs})
target_include_directories(HobbitOperator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (BUILD_TESTS)
    add_gtest(CompositeOp HobbitOperator)
    add_gtest(ElementWiseOp HobbitOperator)
    add_gtest(ReductionOp HobbitOperator)
endif (BUILD_TESTS)