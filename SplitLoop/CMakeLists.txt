include(AddLLVM)

link_directories(${LLVM_LIBRARY_DIRS})

add_library(HobbitIndex include/Index.hpp src/Index.cpp)
target_include_directories(HobbitIndex PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(SplitLoopPass MODULE
        # List your source files here.
        src/SplitLoop.cpp
        )

target_include_directories(SplitLoopPass PUBLIC ${LLVM_INCLUDE_DIRS})

# Use C++11 to compile our pass (i.e., supply -std=c++11).
target_compile_features(SplitLoopPass PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(SplitLoopPass PROPERTIES
        COMPILE_FLAGS "-fno-rtti"
        )

# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
if(APPLE)
    set_target_properties(SplitLoopPass PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
            )
endif(APPLE)

add_custom_target(emit-llvm COMMAND ${CMAKE_CXX_COMPILER} -emit-llvm -c ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp -o test.bc)
add_custom_target(run-opt COMMAND opt -load ${CMAKE_CURRENT_BINARY_DIR}/libSplitLoopPass.so -loop-simplify -loop-idiom -mem2reg -simplifycfg -loop-rotate -split-loop test.bc)
add_dependencies(run-opt emit-llvm SplitLoopPass)