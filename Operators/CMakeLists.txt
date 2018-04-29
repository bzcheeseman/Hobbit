llvm_map_components_to_libnames(llvm_libs all)

file(GLOB UTILS utils/*.h* utils/*.c*)
add_library(HobbitOpUtils SHARED ${UTILS})
target_link_libraries(HobbitOpUtils PRIVATE ${llvm_libs})

function(add_operator name)
    add_library(HobbitOp${name} SHARED registry/${name}.cpp)
    target_include_directories(HobbitOp${name}
            PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/utils ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(HobbitOp${name} PUBLIC HobbitAST HobbitOpUtils ${llvm_libs} Polly PollyISL)

    if(BUILD_TESTS)
        add_gtest(${name} HobbitOp${name})
    endif(BUILD_TESTS)
endfunction()

add_operator(gemm)


#add_custom_target(OpGen COMMAND echo "Done!")
#foreach(FIL IN ITEMS ${SOURCES})
#    get_filename_component(filename ${FIL} NAME_WE)
#
#    add_custom_target(${filename}
#        COMMAND clang -c -S -emit-llvm -O3 -mllvm -disable-llvm-optzns ${FIL} -o ${filename}.ll
#        COMMAND opt -S -polly-canonicalize ${filename}.ll -o ${filename}.preopt.ll
#        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
#
#    add_dependencies(OpGen ${filename})
#endforeach()
