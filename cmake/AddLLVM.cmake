find_package(LLVM 6.0 REQUIRED CONFIG HINTS $ENV{LLVM_POLLY}/lib/cmake/llvm)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_definitions(${LLVM_DEFINITIONS})