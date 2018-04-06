include(AddLLVM)

link_directories(${LLVM_LIBRARY_DIRS})

llvm_map_components_to_libnames(polly_lib polly)

add_library(KernelFusionPass MODULE
        # List your source files here.
        src/KernelFusion.cpp
        )

target_link_libraries(KernelFusionPass PUBLIC ${polly_lib})  # link against polly lib
target_include_directories(KernelFusionPass PUBLIC ${LLVM_INCLUDE_DIRS})

# Use C++11 to compile our pass (i.e., supply -std=c++11).
target_compile_features(KernelFusionPass PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(KernelFusionPass PROPERTIES
        COMPILE_FLAGS "-fno-rtti"
        )

# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
if(APPLE)
    set_target_properties(KernelFusionPass PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
            )
endif(APPLE)
