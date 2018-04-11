find_package(LLVM REQUIRED CONFIG)
find_package(Polly REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

message(STATUS "Found Polly ${Polly_PACKAGE_VERSION}")
message(STATUS "Using PollyConfig.cmake in: ${Polly_DIR}")

add_definitions(${LLVM_DEFINITIONS})
add_definitions(${Polly_DEFINITIONS})