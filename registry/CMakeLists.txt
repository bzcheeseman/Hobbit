llvm_map_components_to_libnames(llvm_libs all)

file(GLOB UTILS ${CMAKE_CURRENT_SOURCE_DIR}/utils/*.h* ${CMAKE_CURRENT_SOURCE_DIR}/utils/*.c*)
add_library(HobbitRegistryUtils SHARED ${UTILS})
target_link_libraries(HobbitRegistryUtils PRIVATE ${llvm_libs})

function(add_operator name)
    add_library(HobbitOp${name} SHARED ${name}.cpp)
    target_include_directories(HobbitOp${name}
            PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
            )
    target_link_libraries(HobbitOp${name} PUBLIC Hobbit HobbitRegistryUtils ${llvm_libs} Polly PollyISL)

    if(BUILD_TESTS)
        add_gtest(${name} HobbitOp${name})
    endif(BUILD_TESTS)
endfunction()

add_operator(gemm)