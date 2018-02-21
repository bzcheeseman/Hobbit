//
// Created by Aman on 2/20/18.
//

#ifndef HOBBIT_VARIABLE_HPP
#define HOBBIT_VARIABLE_HPP

#include <cstdint>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

namespace Hobbit {

  //  llvm::Intrinsic:: is the namespace for intrinsics

  // Think about how this will be used, may make the structure clearer
  class Variable {
  public:
    Variable(llvm::Value *value, uint64_t &ptr_size);

    // As a general rule of thumb this should be used to split the object being
    // wrapped into sse/avx style
    // vectors
    llvm::ArrayRef<llvm::Value *> Pack(llvm::IRBuilder<> &builder,
                                       uint32_t chunk_size);

    // While this one should be used to split into arbitrary chunks - will allow
    // you to further split (or pack) if
    // desired.
    llvm::ArrayRef<Variable *> Split(llvm::IRBuilder<> &builder,
                                     uint32_t chunk_size);

    // Accept llvm::Pointer/llvm::Array

  protected:
    llvm::ArrayRef<llvm::Value *> PackPtrToVector_(llvm::IRBuilder<> &builder,
                                                   llvm::Type *vector_type); // use Alloca instead..
    llvm::ArrayRef<llvm::Value *> PackArrayToVector_(llvm::IRBuilder<> &builder,
                                                     llvm::Type *vector_type);

    llvm::ArrayRef<Variable *> PackPtrToVariable_(llvm::IRBuilder<> &builder,
                                                  llvm::Type *array_type);
    llvm::ArrayRef<Variable *> PackArrayToVariable_(llvm::IRBuilder<> &builder,
                                                    llvm::Type *array_type);

  private:
    llvm::Value *m_value_;
    llvm::Type *m_type_;
    uint64_t ptr_array_size_;
  };

  // Accept C++ pointer and turn it into a llvm constant
  class Constant {
  public:
    Constant(float *ptr, uint64_t &ptr_size);
    Constant(double *ptr, uint64_t &ptr_size);

    Constant(bool *ptr, uint64_t &ptr_size);
    Constant(int8_t *ptr, uint64_t &ptr_size);
    Constant(int16_t *ptr, uint64_t &ptr_size);
    Constant(int32_t *ptr, uint64_t &ptr_size);
    Constant(int64_t *ptr, uint64_t &ptr_size);

    // As a general rule of thumb this should be used to split the object being
    // wrapped into sse/avx style
    // vectors
    llvm::ArrayRef<llvm::Value *> Pack(llvm::IRBuilder<> &builder,
                                              uint32_t chunk_size);

    // While this one should be used to split into arbitrary chunks - will allow
    // you to further split if
    // desired.
    llvm::ArrayRef<Constant *> Split(llvm::IRBuilder<> &builder,
                                   uint32_t chunk_size);

  private:
    llvm::Value *m_value_;
    llvm::Type *m_type_;
    uint64_t ptr_array_size_;
  };
}

#endif // HOBBIT_VARIABLE_HPP
