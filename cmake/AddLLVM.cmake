find_package(LLVM 6.0 REQUIRED CONFIG HINTS /usr/local/Cellar/llvm/6.0.0)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_definitions(${LLVM_DEFINITIONS})