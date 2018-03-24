//
// Created by Aman LaChapelle on 3/22/18.
//
// Hobbit
// Copyright (c) 2018 Aman LaChapelle
// Full license at Hobbit/LICENSE.txt
//

/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    
        http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
 */


#include "Node.hpp"

#include <sstream>

#include <llvm/IR/IRBuilder.h>

Hobbit::ast::Function *Hobbit::ast::Function::Create(const std::string &name) {
  Function *f = new Function;
  f->name_ = name;

  return f;
}

const std::string &Hobbit::ast::Function::GetName() {
  return name_;
}

const std::string &Hobbit::ast::Function::GetSignature() {
  std::stringstream out;
  out << "func " + name_ << " (";
  if (arg_table_.size() > 0) {
    for (int i = 0; i < arg_table_.size()-1; i++) {
      out << arg_table_[i]->GetSignature() << ", ";
    }
    out << (*(arg_table_.end()-1))->GetSignature();
  }
  out << ") {\n";
  if (op_table_.size() > 0) {
    for (auto &op : op_table_) {
      out << "  " << op->GetSignature();
    }
  }
  out << "}";

  signature_ = out.str();
  return signature_;
}

Hobbit::ast::Tensor *Hobbit::ast::Function::GetNewArg(const std::string &name, llvm::SmallVector<uint64_t, 4> dims, llvm::Type *type) {
  // Create a new Tensor
  Tensor *arg = Tensor::CreateVariable(name, this, dims, type);
  // Add the tensor to the arg table
  this->arg_table_.push_back(arg);
  return arg;
}

void Hobbit::ast::Function::PushNode(Hobbit::ast::Node *node) {
  this->op_table_.push_back(node);
}

llvm::Function *Hobbit::ast::Function::EmitFunction(llvm::Module *module) {
  llvm::LLVMContext &ctx = module->getContext();

  llvm::SmallVector<llvm::Type *, 4> arg_types;
  for (auto &arg : arg_table_) {
    arg_types.push_back(arg->GetType());
  }

  llvm::FunctionType *ft =
          llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), arg_types, false);
  llvm::Function *out =
          llvm::cast<llvm::Function>(module->getOrInsertFunction(name_, ft));
  llvm::BasicBlock *entryBB = llvm::BasicBlock::Create(
          ctx, "hobbit." + name_ + ".entry", out);

  // Set up the args properly - modify the pointers that are hanging around everywhere
  llvm::Function::arg_iterator iter = out->arg_begin();
  for (auto &arg : arg_table_) {
    arg->SetBuffer(&(*iter++));
  }

  llvm::BasicBlock *bb = entryBB;
  for (auto &op : op_table_) {
    bb = op->Emit(bb);
  }

  return out;
}

Hobbit::ast::Loop::Loop(const std::string &name, Hobbit::ast::Node *parent, uint64_t range_start, uint64_t range_end)
        : name_(name), parent_(parent), range_start_(range_start), range_end_(range_end) {}

llvm::PHINode *Hobbit::ast::Loop::AddLoopEntry_(llvm::BasicBlock *prev_bb) {
  llvm::LLVMContext &ctx = prev_bb->getContext();
  llvm::Function *parent_func = prev_bb->getParent();

  llvm::BasicBlock *loop_body = llvm::BasicBlock::Create(ctx, this->name_+".body", parent_func);

  llvm::IRBuilder<> builder(prev_bb);
  builder.CreateBr(loop_body);

  builder.SetInsertPoint(loop_body);
  llvm::PHINode *index_var = builder.CreatePHI(builder.getInt64Ty(), 2, this->name_ + ".idx");
  index_var->addIncoming(builder.getInt64(range_start_), prev_bb);

  return index_var;

}

llvm::BasicBlock * Hobbit::ast::Loop::AddLoopExit_(llvm::PHINode *idx_var) {
  llvm::BasicBlock *loop_body = idx_var->getParent();
  llvm::LLVMContext &ctx = idx_var->getContext();
  llvm::Function *parent_func = idx_var->getFunction();

  llvm::BasicBlock *loop_exit = llvm::BasicBlock::Create(ctx, this->name_+".exit", parent_func);

  llvm::IRBuilder<> builder(loop_body);

  llvm::Value *next_idx_var = builder.CreateAdd(idx_var, builder.getInt64(1));
  llvm::Value *end_cond = builder.CreateICmpEQ(next_idx_var, builder.getInt64(range_end_));
  llvm::BranchInst *br = builder.CreateCondBr(end_cond, loop_exit, loop_body);
  this->AddLoopMetadata_(br);

  idx_var->addIncoming(next_idx_var, loop_body);

  builder.SetInsertPoint(loop_exit);
  return loop_exit;
}

void Hobbit::ast::Loop::AddLoopMetadata_(llvm::BranchInst *loop_end_br) {
  llvm::LLVMContext &ctx = loop_end_br->getContext();

  llvm::SmallVector<llvm::Metadata *, 4> args;
  // Reserve operand 0 for loop id self reference.
  auto TempNode = llvm::MDNode::getTemporary(ctx, llvm::None);
  args.push_back(TempNode.get());

  llvm::Metadata *vecMD[] = {
          llvm::MDString::get(ctx, "llvm.loop.vectorize.width"),
          llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(
                  llvm::Type::getInt32Ty(ctx), 8))};
  args.push_back(llvm::MDNode::get(ctx, vecMD));

  llvm::MDNode *LoopID = llvm::MDNode::get(ctx, args);
  LoopID->replaceOperandWith(0, LoopID);

  loop_end_br->setMetadata("llvm.loop", LoopID);
}

Hobbit::ast::Tensor *Hobbit::ast::Tensor::CreateVariable(const std::string &name, Hobbit::ast::Node *parent,
                                                         llvm::SmallVector<uint64_t, 4> dims, llvm::Type *type) {
  Tensor *t = new Tensor(nullptr, std::move(dims), type);
  t->name_ = name;
  t->parent_ = parent;

  return t;
}

Hobbit::ast::Tensor *Hobbit::ast::Tensor::CreateConstant(const std::string &name,
                                                         Node *parent, llvm::SmallVector<uint64_t, 4> dims,
                                                         llvm::Type *type, void *buffer) {
  Tensor *t = CreateVariable(name, parent, std::move(dims), type);
  t->name_ = name;

  uint64_t total_size = t->Size();

  std::vector<llvm::Constant *> buffer_constants;
  if (type->isPointerTy()) {
    type = type->getPointerElementType();
  }

  if (type->isFloatingPointTy()) {
    if (type->isFloatTy()) {
      float *buf = (float *)buffer;
      for (uint64_t i = 0; i < total_size; i++) {
        buffer_constants.push_back(
                llvm::ConstantFP::get(type, (double)buf[i]));
      }
    }
    if (type->isDoubleTy()) {
      double *buf = (double *)buffer;
      for (uint64_t i = 0; i < total_size; i++) {
        buffer_constants.push_back(llvm::ConstantFP::get(type, buf[i]));
      }
    }
  } else if (type->isIntegerTy(64)) {
    uint64_t *buf = (uint64_t *)buffer;
    for (uint64_t i = 0; i < total_size; i++) {
      buffer_constants.push_back(
              llvm::ConstantInt::get(type, buf[i], true));
    }
  } else if (type->isIntegerTy(32)) {
    uint32_t *buf = (uint32_t *)buffer;
    for (uint64_t i = 0; i < total_size; i++) {
      buffer_constants.push_back(
              llvm::ConstantInt::get(type, (uint64_t)buf[i], true));
    }
  } else if (type->isIntegerTy(16)) {
    uint16_t *buf = (uint16_t *)buffer;
    for (uint64_t i = 0; i < total_size; i++) {
      buffer_constants.push_back(
              llvm::ConstantInt::get(type, (uint64_t)buf[i], true));
    }
  } else if (type->isIntegerTy(8)) {
    uint8_t *buf = (uint8_t *)buffer;
    for (uint64_t i = 0; i < total_size; i++) {
      buffer_constants.push_back(
              llvm::ConstantInt::get(type, (uint64_t)buf[i], true));
    }
  } else if (type->isIntegerTy(1)) {
    bool *buf = (bool *)buffer;
    for (uint64_t i = 0; i < total_size; i++) {
      buffer_constants.push_back(
              llvm::ConstantInt::get(type, (uint64_t)buf[i], true));
    }
  }
  else {
    throw std::runtime_error("Unsupported type");
  }

  llvm::ArrayType *arr_type =
          llvm::ArrayType::get(type, buffer_constants.size());

  t->llvm_buffer_ = llvm::ConstantArray::get(arr_type, buffer_constants);
  buffer_constants.clear();

  return t;

}

Hobbit::ast::Node *Hobbit::ast::Tensor::GetParent() {
  return parent_;
}

llvm::Type *Hobbit::ast::Tensor::GetType() {
  return llvm_type_;
}

void Hobbit::ast::Tensor::SetBuffer(llvm::Value *val) {
  this->llvm_buffer_ = val;
  this->llvm_type_ = val->getType();

  // set the name of the buffer
  this->llvm_buffer_->setName(this->name_);
}

uint64_t Hobbit::ast::Tensor::NDim() {
  return dims_.size();
}

uint64_t Hobbit::ast::Tensor::Dim(uint64_t which) {
  return dims_[which];
}

uint64_t Hobbit::ast::Tensor::Size() {
  uint64_t total_size = 1;
  for (auto &dim : dims_) {
    total_size *= dim;
  }
  return total_size;
}

Hobbit::ast::Tensor *
Hobbit::ast::Tensor::Chip(llvm::BasicBlock *BB, llvm::SmallVector<uint64_t, 4> start_idx, llvm::SmallVector<uint64_t, 4> dims) {
  // create a new tensor that aliases this
  Tensor *chip = new Tensor(this->GEP(BB, std::move(start_idx)), std::move(dims), this->llvm_type_);
  chip->parent_ = this;

  return chip;
}

Hobbit::ast::Tensor *Hobbit::ast::Tensor::Chip(llvm::BasicBlock *BB, llvm::SmallVector<llvm::Value *, 4> start_idx,
                                               llvm::SmallVector<uint64_t, 4> dims) {
  // create a new tensor that aliases this
  Tensor *chip = new Tensor(this->GEP(BB, start_idx), dims, this->llvm_type_);
  chip->parent_ = this->parent_;

  return chip;
}

Hobbit::ast::Tensor *Hobbit::ast::Tensor::Flatten() {
  this->dims_ = {this->Size()};
  return this;
}

llvm::Value *Hobbit::ast::Tensor::GEP(llvm::BasicBlock *BB, llvm::SmallVector<uint64_t, 4> idx) {
  llvm::IRBuilder<> builder(BB);
  llvm::Value *idx_val = builder.getInt64(this->At_(std::move(idx)));

  return builder.CreateGEP(llvm_buffer_, idx_val);
}

llvm::Value *Hobbit::ast::Tensor::GEP(llvm::BasicBlock *BB, llvm::SmallVector<llvm::Value *, 4> idx) {
  llvm::IRBuilder<> builder(BB);
  llvm::Value *idx_val = this->AtVal_(BB, idx);

  return builder.CreateGEP(llvm_buffer_, idx_val);
}

llvm::Value *Hobbit::ast::Tensor::GEP(llvm::BasicBlock *BB, uint64_t raw_idx) {
  llvm::IRBuilder<> builder(BB);
  llvm::Value *idx_val = builder.getInt64(raw_idx);

  return builder.CreateGEP(llvm_buffer_, idx_val);
}

llvm::Value *Hobbit::ast::Tensor::GEP(llvm::BasicBlock *BB, llvm::Value *raw_idx) {
  llvm::IRBuilder<> builder(BB);

  return builder.CreateGEP(llvm_buffer_, raw_idx);
}

llvm::Value *Hobbit::ast::Tensor::Load(llvm::BasicBlock *BB, llvm::SmallVector<uint64_t, 4> idx) {
  llvm::Value *gep = this->GEP(BB, std::move(idx));
  llvm::IRBuilder<> builder(BB);

  return builder.CreateAlignedLoad(gep, ALIGNMENT);
}

llvm::Value *Hobbit::ast::Tensor::Load(llvm::BasicBlock *BB, llvm::SmallVector<llvm::Value *, 4> idx) {
  llvm::Value *gep = this->GEP(BB, idx);
  llvm::IRBuilder<> builder(BB);

  return builder.CreateAlignedLoad(gep, ALIGNMENT);
}

llvm::Value *Hobbit::ast::Tensor::Load(llvm::BasicBlock *BB, uint64_t raw_idx) {
  llvm::Value *gep = this->GEP(BB, raw_idx);
  llvm::IRBuilder<> builder(BB);

  return builder.CreateAlignedLoad(gep, ALIGNMENT);
}

llvm::Value *Hobbit::ast::Tensor::Load(llvm::BasicBlock *BB, llvm::Value *raw_idx) {
  llvm::Value *gep = this->GEP(BB, raw_idx);
  llvm::IRBuilder<> builder(BB);

  return builder.CreateAlignedLoad(gep, ALIGNMENT);
}

Hobbit::ast::Tensor::Tensor(llvm::Value *ptr, llvm::SmallVector<uint64_t, 4> dims, llvm::Type *type)
        : llvm_buffer_(ptr), dims_(std::move(dims)), llvm_type_(type) {}

uint64_t Hobbit::ast::Tensor::At_(llvm::SmallVector<uint64_t, 4> idx) {
  if (idx.size() != dims_.size()) throw std::runtime_error("Incorrect number of indices");

  uint64_t out = 0;
  for (uint64_t i = 0; i < dims_.size()-1; i++) {
    out += idx[i];
    out *= dims_[i+1];
  }
  out += *idx.end();

  return out;
}

llvm::Value *Hobbit::ast::Tensor::AtVal_(llvm::BasicBlock *BB, llvm::SmallVector<llvm::Value *, 4> idx) {
  llvm::IRBuilder<> builder(BB);
  llvm::Value *out = builder.getInt64(0);

  for (uint64_t i = 0; i < dims_.size()-1; i++) {
    out = builder.CreateAdd(out, idx[i]);
    out = builder.CreateMul(out, builder.getInt64(dims_[i+1]));
  }
  out = builder.CreateAdd(out, *idx.end());

  return out;
}

const std::string &Hobbit::ast::Tensor::GetName() {
  return name_;
}

const std::string &Hobbit::ast::Tensor::GetSignature() {
  std::stringstream out;
  out << "tensor: " << name_ << " (";
  for (auto &dim : dims_) {
    out << dim << ", ";
  }
  out << ")";

  signature_ = out.str();
  return signature_;
}

Hobbit::ast::HSum *Hobbit::ast::HSum::Create(const std::string &name, Hobbit::ast::Node *parent, uint64_t range_start,
                                             uint64_t range_end) {
  HSum *out = new HSum(name, parent, range_start, range_end);
  return out;
}

Hobbit::ast::HSum::HSum(const std::string &name, Hobbit::ast::Node *parent, uint64_t range_start, uint64_t range_end)
        : Loop(name, parent, range_start, range_end) {
  ;
}

void Hobbit::ast::HSum::AddKernel(llvm::SmallVector<llvm::PHINode *, 4> idx_vars) {
  llvm::PHINode *idx = idx_vars[0];
  llvm::LLVMContext &ctx = idx->getContext();
  llvm::BasicBlock *bb = idx->getParent();

  Tensor *arg = args_[0];

  llvm::IRBuilder<> builder(bb);
  llvm::Type *accumulator_type = arg->GetType();
  if (accumulator_type->isPointerTy()) {
    accumulator_type = accumulator_type->getPointerElementType();
  }

  llvm::Value *zero = builder.CreateBitCast(builder.getInt64(0), accumulator_type);

  llvm::PHINode *accumulator = builder.CreatePHI(accumulator_type, 2, name_ + ".accumulator");
  accumulator->addIncoming(zero, bb->getSinglePredecessor());

  llvm::Value *accumulator_next;
  if (accumulator_type->isIntegerTy()) {
    accumulator_next =
            builder.CreateAdd(accumulator, arg->Load(bb, idx));
  }
  if (accumulator_type->isFloatingPointTy()) {
    accumulator_next =
            builder.CreateFAdd(accumulator, arg->Load(bb, idx));
  }

  accumulator->addIncoming(accumulator_next, bb);
}

const std::string &Hobbit::ast::HSum::GetName() {
  return name_;
}

const std::string &Hobbit::ast::HSum::GetSignature() {
  std::stringstream out;

  out << name_ << " (" << args_[0]->GetName() << ") "
          "redux dim: " << dim_idx_
      << " range: (" << range_start_ << ", " << range_end_ << ")" << "\n";
  signature_ = out.str();
  return signature_;
}

Hobbit::ast::Tensor *Hobbit::ast::HSum::SetArgs(llvm::SmallVector<Tensor *, 2> args) {
  if (args.size() != 1) throw std::runtime_error("Too many arguments passed to "+name_);

  args_ = std::move(args);

  uint64_t range = range_end_ - range_start_;
  dim_idx_ = UINT64_MAX;
  for (uint64_t i = 0; i < args_[0]->NDim(); i++) {
    if (range == args_[0]->Dim(i)) {
      dim_idx_ = i;
      break;
    }
  }

  if (dim_idx_ == UINT64_MAX) throw std::runtime_error("No matching dimension to the loop range");

  out_.push_back(Tensor::CreateVariable(name_+".output", this, {1}, args_[0]->GetType()));

  return out_[0];
}

llvm::BasicBlock *Hobbit::ast::HSum::Emit(llvm::BasicBlock *prev_bb)  {
  llvm::PHINode *idx_var = this->AddLoopEntry_(prev_bb);
  this->AddKernel({idx_var});
  return this->AddLoopExit_(idx_var);
}
