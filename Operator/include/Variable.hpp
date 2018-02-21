#ifndef HOBBIT_VARIABLE_HPP
#define HOBBIT_VARIABLE_HPP


#include <cstdint>
#include <llvm/IR/Value.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/IRBuilder.h>
#include <list>

namespace Hobbit {

//  llvm::Intrinsic:: is the namespace for intrinsics

  struct Range {
    Range(const uint32_t &start=0, const uint32_t &end=0) : start(start), end(end) {}

    uint32_t start;
    uint32_t end; // exclusive
  };

  enum Axis {
    K,
    H,
    W
  };

  // TODO: make dynamic batch size possible with n = 0?
  // TODO: Check indexing to make sure everything is correct
  class Shape {
  public:
    Shape(const uint32_t &k, const uint32_t &h, const uint32_t &w);

    uint32_t At(const uint32_t &k, const uint32_t &h, const uint32_t &w);

    Shape GetChunkShape(const Range &k_range, const Range &h_range, const Range &w_range);
    Range GetChunkIdx(const Range &k_range, const Range &h_range, const Range &w_range);

    uint32_t GetSize();

    const uint32_t &GetAxisSize(Axis &axis);


  private:
    const uint32_t k_, h_, w_;
  };

  class Variable {
  public:

    // Takes in the shape of the buffer to allocate, allows us to do things like Pack and Split
    Variable(llvm::IRBuilder<> &builder, llvm::Type *scalar_type, const Shape &shape);

    Variable(llvm::Value *value, llvm::Type *type, const Shape &shape, Variable *parent);

    virtual ~Variable();

    // Should be used to split into arbitrary chunks - will allow you to further split if desired.
    Variable *GetChunk(llvm::IRBuilder<> &builder, const Range &k_range, const Range &h_range, const Range &w_range);
    Variable *Flatten();

    void ClearChild(Variable *child);
    void ClearChildren();

    // This function treats the whole array as a flat buffer, and is therefore recommended to only be used
    // when trying to schedule a computation using SIMD instructions, and therefore used on a very small
    // piece of a variable.
    llvm::ArrayRef<llvm::Value *> Pack(llvm::IRBuilder<> &builder, uint32_t &vector_size);

  protected:
    llvm::Value *m_value_;
    llvm::Type *m_type_;
    Shape m_shape_;

    Variable *m_parent_ = nullptr;
    std::list<Variable *> m_children_;
  };

  // TODO: Constant object with the same methods as Variable
}


#endif //HOBBIT_VARIABLE_HPP
