function(add_pass name)
    add_library(${name}Pass MODULE
            ${name}/src/${name}.cpp
    )
    target_include_directories(${name}Pass
            PUBLIC ${LLVM_INCLUDE_DIRS}
            PUBLIC ${LLVM_INCLUDE_DIRS}/polly
    )

    # Use C++11 to compile our pass (i.e., supply -std=c++11).
    target_compile_features(${name}Pass PRIVATE cxx_range_for cxx_auto_type)

    # LLVM is (typically) built with no C++ RTTI. We need to match that;
    # otherwise, we'll get linker errors about missing RTTI data.
    set_target_properties(${name}Pass PROPERTIES
            COMPILE_FLAGS "-fno-rtti"
    )

    # Get proper shared-library behavior (where symbols are not necessarily
    # resolved when the shared library is linked) on OS X.
    if(APPLE)
        set_target_properties(${name}Pass PROPERTIES
                LINK_FLAGS "-undefined dynamic_lookup"
        )
    endif(APPLE)
endfunction()

add_pass(KernelFusion)
