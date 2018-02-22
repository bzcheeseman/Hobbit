#ifndef HOBBIT_VARIABLE_HPP
#define HOBBIT_VARIABLE_HPP

#include <cstdint>
#include <list>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/TypeBuilder.h>

namespace Hobbit {

  //  llvm::Intrinsic:: is the namespace for intrinsics

  struct Range {
    explicit Range(const uint32_t &start = 0, const uint32_t &end = 0)
        : start(start), end(end) {}

    uint32_t start;
    uint32_t end; // exclusive
  };

  enum Axis { K, H, W };

  class Shape {
  public:
    Shape(const uint32_t &k, const uint32_t &h, const uint32_t &w);

    uint32_t At(const uint32_t &k, const uint32_t &h, const uint32_t &w);

    Shape GetChunkShape(const Range &k_range, const Range &h_range,
                        const Range &w_range);
    Range GetChunkIdx(const Range &k_range, const Range &h_range,
                      const Range &w_range);

    uint32_t GetSize() const;

    const uint32_t &GetAxisSize(Axis &axis);

  private:
    const uint32_t k_, h_, w_;
  };

  class Buffer {
  public:
    // Takes in the shape of the buffer to allocate, allows us to do things like
    // Pack and Split
    Buffer(llvm::IRBuilder<> &builder, llvm::Type *scalar_type,
             const Shape &shape);

    Buffer(llvm::Value *value, llvm::Type *type, const Shape &shape,
             Buffer *parent);

    Buffer(llvm::Value *value, const Shape &shape);

    virtual ~Buffer();

    const Shape &GetShape();

    // Should be used to split into arbitrary chunks - will allow you to further
    // split if desired.
    Buffer *GetChunk(llvm::IRBuilder<> &builder, const Range &k_range,
                       const Range &h_range, const Range &w_range);
    Buffer *Flatten();

    void ClearChild(Buffer *child);
    void ClearChildren();

    // This function treats the whole array as a flat buffer, and is therefore
    // recommended to only be used
    // when trying to schedule a computation using SIMD instructions, and
    // therefore used on a very small
    // piece of a variable.
    llvm::ArrayRef<llvm::Value *> Pack(llvm::IRBuilder<> &builder,
                                       uint32_t &vector_size);

  protected:
    llvm::Value *m_value_;
    llvm::Type *m_type_;
    Shape m_shape_;

    Buffer *m_parent_ = nullptr;
    std::list<Buffer *> m_children_;
  };

  class CompileTimeBuffer {
  public:
    virtual const Shape &GetShape() = 0;
    virtual llvm::Value *GetValue(llvm::IRBuilder<> &builder, llvm::Type *type) = 0;
  };

  class CompileTimeIntBuffer : public CompileTimeBuffer {
  public:
    CompileTimeIntBuffer(uint64_t *ptr, const Shape &shape) : ptr_(ptr), shape_(shape) {}

    const Shape &GetShape() override {
      return shape_;
    }

    llvm::Value *GetValue(llvm::IRBuilder<> &builder, llvm::Type *int_type) override {
      llvm::Type *type = int_type;

      if (int_type == nullptr) type = builder.getInt16Ty();

      uint32_t size = shape_.GetSize();

      llvm::Value *alloca_size = builder.getInt32(size);
      uint32_t bit_width = type->getIntegerBitWidth();

      llvm::Value *out = builder.CreateAlloca(type, alloca_size);

      for (uint32_t i = 0; i < size; i++) {
        llvm::Value *elt = builder.getInt(llvm::APInt(bit_width, ptr_[i]));
        builder.CreateStore(elt, builder.CreateGEP(out, builder.getInt32(i)));
      }

      return out;
    }

  private:
    uint64_t *ptr_;
    const Shape shape_;
  };

  class CompileTimeFPBuffer : public CompileTimeBuffer {
  public:
    CompileTimeFPBuffer(double *ptr, const Shape &shape) : ptr_(ptr), shape_(shape) {}

    const Shape &GetShape() override {
      return shape_;
    }

    llvm::Value *GetValue(llvm::IRBuilder<> &builder, llvm::Type *fp_type) override {
      llvm::Type *type = fp_type;

      if (fp_type == nullptr) type = builder.getFloatTy();

      uint32_t size = shape_.GetSize();

      llvm::Value *alloca_size = builder.getInt32(size);

      llvm::Value *out = builder.CreateAlloca(type, alloca_size);

      for (uint32_t i = 0; i < size; i++) {
        llvm::Value *elt = llvm::ConstantFP::get(type, ptr_[i]);
        builder.CreateStore(elt, builder.CreateGEP(out, builder.getInt32(i)));
      }

      return out;
    }

  private:
    double *ptr_;
    const Shape shape_;
  };

  class Constant : public Buffer {
  public:
    Constant(llvm::IRBuilder<> &builder, llvm::Type *scalar_type, CompileTimeBuffer &buffer);
  };

  class Variable : public Buffer {
  public:
    Variable(llvm::IRBuilder<> &builder, llvm::Type *scalar_type, const Shape &shape);
  };
}

#endif // HOBBIT_VARIABLE_HPP
