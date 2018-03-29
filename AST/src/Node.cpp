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
#include "Visitor.hpp"

#include <sstream>

#include <glog/logging.h>

#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>

Hobbit::Function *Hobbit::Function::Create(const std::string &name) {
  Function *f = new Function;
  f->name_ = "hobbit." + name;
  f->child_ = nullptr;
  f->parent_ = nullptr;

  return f;
}

Hobbit::Tensor *Hobbit::Function::GetNewArg(const std::string &name,
                                            llvm::SmallVector<uint64_t, 4> dims,
                                            llvm::Type *type) {
  // Create a new Tensor
  Tensor *arg = Tensor::CreateVariable(name, this, std::move(dims), type);
  // Add the tensor to the arg table
  arg_table_.push_back(arg);
  return arg;
}

Hobbit::Tensor *
Hobbit::Function::GetNewAlloca(const std::string &name,
                               llvm::SmallVector<uint64_t, 4> dims,
                               llvm::Type *type) {
  // Create a new Tensor
  Tensor *arg = Tensor::CreateVariable(name, this, std::move(dims), type);
  // Add the tensor to the arg table
  alloca_table_.push_back(arg);
  return arg;
}

 Hobbit::Tensor *
 Hobbit::Function::GetNewArg(const std::string &name,
 llvm::SmallVector<uint64_t, 4> dims, llvm::Type *type,
                                 void *buffer) {
  // Create a new Tensor
  Tensor *arg = Tensor::CreateConstant(name, this, std::move(dims), type,
  buffer);
  // Add the tensor to the arg table
  arg_table_.push_back(arg);
  return arg;
}

void Hobbit::Function::SetArg(Hobbit::Tensor *t) {
  // If we've already added this arg to the alloca table then erase it - it
  // should be an arg
  for (auto &alloca : alloca_table_) {
    if (t == alloca)
      alloca_table_.erase(&alloca);
  }
  arg_table_.push_back(t);
}

void Hobbit::Function::PushNode(Hobbit::ast::Node *node) {
  if (child_ == nullptr) {
    child_ = std::move(node);
    last_node_ = child_;
    return;
  }

  // Update the child of the last node
  last_node_->SetChild(node);
  // Update the last node
  last_node_ = last_node_->GetChild();
}

void Hobbit::Function::Emit(Hobbit::Visitor *CG) {
  llvm::LLVMContext *ctx = CG->GetContext();
  llvm::Module *module = CG->GetModule();

  llvm::SmallVector<llvm::Type *, 4> arg_types;
  for (auto &arg : arg_table_) {
    arg_types.push_back(arg->GetType());
  }

  llvm::FunctionType *ft =
      llvm::FunctionType::get(llvm::Type::getVoidTy(*ctx), arg_types, false);

  llvm::Function *out =
      llvm::cast<llvm::Function>(module->getOrInsertFunction(name_, ft));
  // Create entry block, do nothing with it
  llvm::BasicBlock *entry_bb =
      llvm::BasicBlock::Create(*ctx, name_ + ".entry", out);
  llvm::IRBuilder<> builder(entry_bb);

  llvm::Function::arg_iterator iter = out->arg_begin();
  for (auto &arg : arg_table_) {
    arg->SetBuffer(&(*iter++));
  }

  for (auto &alloca : alloca_table_) {
    llvm::Type *alloca_type = alloca->GetType();
    if (alloca_type->isPointerTy())
      alloca_type = alloca_type->getPointerElementType();
    llvm::Value *a =
        builder.CreateAlloca(alloca_type, builder.getInt64(alloca->Size()));
    alloca->SetBuffer(a);
  }

  CG->PushFunction(this, out); // keyed on AST function

  Node *n = child_;
  while (n != nullptr) {
    n->Emit(CG);
    n = n->GetChild();
  }

  out = CG->GetFunction(this);

  // Create a return for the exit BB
  llvm::BasicBlock *exit_bb = &*(--out->end());
  builder.SetInsertPoint(exit_bb);
  builder.CreateRetVoid();

  // Finalize the entry BB if that hasn't already happened
  entry_bb = &out->getEntryBlock();
  llvm::BranchInst *br;
  for (auto &instr : *entry_bb) {
    br = llvm::dyn_cast<llvm::BranchInst>(&instr);
    if (br)
      break;
  }
  if (br) {
    llvm::outs() << "Branch inst found\n";
    br->moveAfter(&*--entry_bb->end());
    return;
  }
  llvm::outs() << "No branch inst found\n";
  builder.SetInsertPoint(entry_bb);
  builder.CreateBr(&*(++out->begin()));
}

Hobbit::Loop::Loop(const std::string &name, Hobbit::ast::Node *parent,
                   uint64_t range_start, uint64_t range_end, uint64_t stride, uint64_t chunk_size)
    : name_(name), Node(parent, nullptr), range_start_(range_start),
      range_end_(range_end), stride_(stride), chunk_size_(chunk_size) {}

void Hobbit::Loop::AddLoopEntry_(llvm::BasicBlock *prev_bb) {
  llvm::LLVMContext &ctx = prev_bb->getContext();
  llvm::Function *parent_func = prev_bb->getParent();

  llvm::BasicBlock *loop_body =
      llvm::BasicBlock::Create(ctx, this->name_ + ".body", parent_func);
  llvm::BasicBlock *chunked_body = nullptr;
  llvm::BasicBlock *strided_body = nullptr;

  llvm::IRBuilder<> builder(prev_bb);
  builder.CreateBr(loop_body);

  builder.SetInsertPoint(loop_body);
  loop_phi_incr_["base"].phi =
      builder.CreatePHI(builder.getInt64Ty(), 2, this->name_ + ".idx");
  loop_phi_incr_["base"].phi->addIncoming(builder.getInt64(range_start_), prev_bb);
  loop_phi_incr_["base"].increment = builder.getInt64(1);
  loop_phi_incr_["base"].start = range_start_;
  loop_phi_incr_["base"].end = range_end_;

  if (stride_ > 1) strided_body = llvm::BasicBlock::Create(ctx, this->name_ + ".body.strided", parent_func);
  if (chunk_size_ > 1) chunked_body = llvm::BasicBlock::Create(ctx, this->name_ + ".body.chunked", parent_func);

  if (stride_ > 1) {
    if (!chunked_body) builder.SetInsertPoint(loop_body);
    if (chunked_body) builder.SetInsertPoint(chunked_body);
    builder.CreateBr(strided_body);
    builder.SetInsertPoint(strided_body);

    loop_phi_incr_["strided"].phi = builder.CreatePHI(builder.getInt64Ty(), 2, this->name_ + ".stride.idx");
    loop_phi_incr_["strided"].phi->addIncoming(builder.getInt64(0), loop_body);
    loop_phi_incr_["strided"].increment = builder.getInt64(1);
    loop_phi_incr_["strided"].start = 0;
    loop_phi_incr_["strided"].end = stride_;

    builder.SetInsertPoint(loop_body);
    loop_phi_incr_["base"].increment = builder.getInt64(stride_);
  }

  if (chunk_size_ > 1) {
    builder.SetInsertPoint(loop_body);
    builder.CreateBr(chunked_body);
    builder.SetInsertPoint(chunked_body);

    loop_phi_incr_["chunked"].phi = builder.CreatePHI(builder.getInt64Ty(), 2, this->name_ + ".chunk.idx");
    loop_phi_incr_["chunked"].phi->addIncoming(builder.getInt64(0), loop_body);
    loop_phi_incr_["chunked"].increment = builder.getInt64(stride_);
    loop_phi_incr_["chunked"].start = 0;
    loop_phi_incr_["chunked"].end = chunk_size_*stride_;

    builder.SetInsertPoint(loop_body);
    loop_phi_incr_["base"].increment = builder.getInt64(stride_*chunk_size_);
  }
}

llvm::BasicBlock *Hobbit::Loop::AddLoopExit_() {

  llvm::PHINode *base = loop_phi_incr_["base"].phi;
  llvm::Value *base_incr = loop_phi_incr_["base"].increment;

  llvm::BasicBlock *base_body = base->getParent();
  llvm::LLVMContext &ctx = base->getContext();
  llvm::Function *parent_func = base->getFunction();

  llvm::BasicBlock *base_exit = llvm::BasicBlock::Create(ctx, this->name_ + ".exit", parent_func);
  llvm::BasicBlock *chunk_exit = nullptr;
  llvm::BasicBlock *stride_exit = nullptr;

  llvm::IRBuilder<> builder(base_body);

  if (chunk_size_ > 1) {
    llvm::PHINode *chunked = loop_phi_incr_["chunked"].phi;
    llvm::Value *chunked_incr = loop_phi_incr_["chunked"].increment;

    llvm::BasicBlock *chunked_body = chunked->getParent();
    builder.CreateBr(chunked_body);

    chunk_exit = llvm::BasicBlock::Create(ctx, this->name_ + ".chunk.exit", parent_func);

    llvm::IRBuilder<> builder(chunked_body);
    llvm::Value *next_idx_var = builder.CreateAdd(chunked, chunked_incr);
    llvm::Value *end_cond =
            builder.CreateICmpEQ(next_idx_var, builder.getInt64(loop_phi_incr_["chunked"].end));
    llvm::BranchInst *br = builder.CreateCondBr(end_cond, chunk_exit, chunked_body);
    this->AddLoopMetadata_(br);

    chunked->addIncoming(next_idx_var, chunked_body);

  }

  if (stride_ > 1) {
    llvm::PHINode *strided = loop_phi_incr_["strided"].phi;
    llvm::Value *strided_incr = loop_phi_incr_["strided"].increment;

    llvm::BasicBlock *strided_body = strided->getParent();
    builder.CreateBr(strided_body);

    stride_exit = llvm::BasicBlock::Create(ctx, this->name_ + ".stride.exit", parent_func);

    llvm::IRBuilder<> builder(strided_body);
    llvm::Value *next_idx_var = builder.CreateAdd(strided, strided_incr);
    llvm::Value *end_cond =
      builder.CreateICmpEQ(next_idx_var, builder.getInt64(loop_phi_incr_["strided"].end));
    llvm::BranchInst *br = builder.CreateCondBr(end_cond, stride_exit, strided_body);
    this->AddLoopMetadata_(br);

    strided->addIncoming(next_idx_var, strided_body);
  }

  llvm::Value *next_idx_var = builder.CreateAdd(base, base_incr);
  llvm::Value *end_cond =
          builder.CreateICmpEQ(next_idx_var, builder.getInt64(loop_phi_incr_["base"].end));
  llvm::BranchInst *br = builder.CreateCondBr(end_cond, base_exit, base_body);
  this->AddLoopMetadata_(br);
  base->addIncoming(next_idx_var, base_body);

//  llvm::BasicBlock *loop_body = idx->getParent();
//  llvm::LLVMContext &ctx = idx->getContext();
//  llvm::Function *parent_func = idx->getFunction();
//
//  llvm::BasicBlock *loop_exit =
//          llvm::BasicBlock::Create(ctx, this->name_ + ".exit", parent_func);
//
//  llvm::IRBuilder<> builder(loop_body);
//
//
//  llvm::Value *next_idx_var = builder.CreateAdd(idx, incr);
//  llvm::Value *end_cond =
//      builder.CreateICmpEQ(next_idx_var, builder.getInt64(chunk_size_));
//  llvm::BranchInst *br = builder.CreateCondBr(end_cond, loop_exit, loop_body);
//  this->AddLoopMetadata_(br);
//
//  loop_phi_incr_["chunked"].first->addIncoming(next_idx_var, loop_body);

  builder.SetInsertPoint(base_exit);
  return base_exit;
}

void Hobbit::Loop::AddLoopMetadata_(llvm::BranchInst *loop_end_br) {
  llvm::LLVMContext &ctx = loop_end_br->getContext();

  llvm::SmallVector<llvm::Metadata *, 4> args;
  // Reserve operand 0 for loop id self reference.
  auto TempNode = llvm::MDNode::getTemporary(ctx, llvm::None);
  args.push_back(TempNode.get());

  llvm::Metadata *vecMD[] = {
      llvm::MDString::get(ctx, "llvm.loop.vectorize.width"),
      llvm::ConstantAsMetadata::get(
          llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx), 8))};
  llvm::Metadata *unrollMD[] = {
      llvm::MDString::get(ctx, "llvm.loop.unroll.count"),
      llvm::ConstantAsMetadata::get(
          llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx), 8))};
  args.push_back(llvm::MDNode::get(ctx, vecMD));
  args.push_back(llvm::MDNode::get(ctx, unrollMD));

  llvm::MDNode *LoopID = llvm::MDNode::get(ctx, args);
  LoopID->replaceOperandWith(0, LoopID);

  loop_end_br->setMetadata("llvm.loop", LoopID);
}

Hobbit::HSum *Hobbit::HSum::Create(const std::string &name,
                                   Hobbit::ast::Node *parent,
                                   uint64_t range_start, uint64_t range_end) {
  HSum *out = new HSum("hobbit." + name, parent, range_start, range_end);
  return out;
}

Hobbit::HSum::HSum(const std::string &name, Hobbit::ast::Node *parent,
                   uint64_t range_start, uint64_t range_end)
    : Loop(name, parent, range_start, range_end) {
  ;
}

Hobbit::Tensor *Hobbit::HSum::SetArgs(llvm::SmallVector<Tensor *, 2> args) {
  if (args.size() != 1)
    LOG(FATAL) << "Too many arguments passed to " + name_;
  if (args[0]->NDim() != 1)
    LOG(WARNING) << "Treating input tensor as flat within " + name_;

  args_ = std::move(args);
  args_[0] = args_[0]->Flatten();

  uint64_t range = range_end_ - range_start_;
  if (args_[0]->Dim(0) != range)
    LOG(FATAL) << "Incorrect size passed in " + name_;

  llvm::Type *output_type = args_[0]->GetType();
  if (output_type->isArrayTy()) {
    output_type = output_type->getArrayElementType()->getPointerTo(0);
  }

  Tensor *output = llvm::dyn_cast<Function>(parent_)->GetNewAlloca(
      name_ + ".output", {1}, output_type);

  out_.push_back(output);

  return out_[0];
}

void Hobbit::HSum::Emit(Hobbit::Visitor *CG) {
  llvm::Function *f;
  Node *n = parent_;
  Function *parent;
  while (!(parent = llvm::dyn_cast<Function>(n))) {
    n = n->GetParent();
  }
  f = CG->GetFunction(parent); // takes the parent function of this node

  llvm::BasicBlock *prev_bb = &(*(--f->end()));
  this->AddLoopEntry_(prev_bb);

//  llvm::LLVMContext &ctx = loop_phi_incr_["base"].phi->getContext();
//  llvm::BasicBlock *bb = loop_phi_incr_["base"].phi->getParent();
//
//  Tensor *arg = args_[0];
//
//  llvm::Type *accumulator_type = arg->GetType();
//  if (accumulator_type->isPointerTy()) {
//    accumulator_type = accumulator_type->getPointerElementType();
//  }
//  if (accumulator_type->isArrayTy()) {
//    accumulator_type = accumulator_type->getArrayElementType();
//  }
//
//  llvm::IRBuilder<> builder(&bb->getParent()->getEntryBlock());
//
//  llvm::Value *zero =
//          builder.CreateBitCast(builder.getInt64(0), accumulator_type);
//
//  out_[0]->Store(builder.GetInsertBlock(), (uint64_t)0, zero);
//
//  builder.SetInsertPoint(bb);
//
//  llvm::Value *accumulator_next;
//  if (accumulator_type->isIntegerTy()) {
//    accumulator_next =
//            builder.CreateAdd(out_[0]->Load(bb, (uint64_t)0), arg->Load(bb, loop_phi_incr_["base"].phi));
//  }
//  if (accumulator_type->isFloatingPointTy()) {
//    accumulator_next =
//            builder.CreateFAdd(out_[0]->Load(bb, (uint64_t)0), arg->Load(bb, loop_phi_incr_["base"].phi));
//  }
//
//  out_[0]->Store(bb, (uint64_t)0, accumulator_next);

  this->AddLoopExit_();
}

Hobbit::Tensor *Hobbit::Tensor::CreateVariable(
    const std::string &name, Hobbit::ast::Node *parent,
    llvm::SmallVector<uint64_t, 4> dims, llvm::Type *type) {
  Tensor *t = new Tensor(nullptr, std::move(dims), type);
  t->name_ = name;
  t->parent_ = parent;

  return t;
}

Hobbit::Tensor *
Hobbit::Tensor::CreateConstant(const std::string &name, Node *parent,
                                  llvm::SmallVector<uint64_t, 4> dims,
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
        buffer_constants.push_back(llvm::ConstantFP::get(type,
        (double)buf[i]));
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
      buffer_constants.push_back(llvm::ConstantInt::get(type, buf[i], true));
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
  } else {
    LOG(FATAL) << "Unsupported type";
  }

  llvm::ArrayType *arr_type =
      llvm::ArrayType::get(type, buffer_constants.size());
  t->llvm_buffer_ = llvm::ConstantArray::get(arr_type, buffer_constants);
  t->llvm_type_ = t->llvm_buffer_->getType();
  buffer_constants.clear();

  return t;
}

llvm::Type *Hobbit::Tensor::GetType() { return llvm_type_; }

void Hobbit::Tensor::SetBuffer(llvm::Value *val) {
  this->llvm_buffer_ = val;
  this->llvm_type_ = val->getType();

  // set the name of the buffer
  this->llvm_buffer_->setName(this->name_);
}

llvm::Value *Hobbit::Tensor::GetBuffer() { return llvm_buffer_; }

uint64_t Hobbit::Tensor::NDim() { return dims_.size(); }

uint64_t Hobbit::Tensor::Dim(uint64_t which) { return dims_[which]; }

uint64_t Hobbit::Tensor::Size() {
  uint64_t total_size = 1;
  for (auto &dim : dims_) {
    total_size *= dim;
  }
  return total_size;
}

Hobbit::Tensor *Hobbit::Tensor::Chip(llvm::BasicBlock *BB,
                                     llvm::SmallVector<uint64_t, 4> start_idx,
                                     llvm::SmallVector<uint64_t, 4> dims) {
  // create a new tensor that aliases this
  Tensor *chip = new Tensor(this->GEP(BB, std::move(start_idx)),
                            std::move(dims), this->llvm_type_);
  chip->parent_ = this;
  this->child_ = chip;

  return chip;
}

Hobbit::Tensor *
Hobbit::Tensor::Chip(llvm::BasicBlock *BB,
                     llvm::SmallVector<llvm::Value *, 4> start_idx,
                     llvm::SmallVector<uint64_t, 4> dims) {
  // create a new tensor that aliases this
  Tensor *chip = new Tensor(this->GEP(BB, start_idx), dims, this->llvm_type_);
  chip->parent_ = this;
  this->child_ = chip;

  return chip;
}

Hobbit::Tensor *Hobbit::Tensor::Flatten() {
  this->dims_ = {this->Size()};
  return this;
}

llvm::Value *Hobbit::Tensor::GEP(llvm::BasicBlock *BB,
                                 llvm::SmallVector<uint64_t, 4> idx) {
  llvm::IRBuilder<> builder(BB);
  llvm::Value *idx_val = builder.getInt64(this->At_(std::move(idx)));

  return builder.CreateInBoundsGEP(llvm_buffer_, idx_val);
}

llvm::Value *Hobbit::Tensor::GEP(llvm::BasicBlock *BB,
                                 llvm::SmallVector<llvm::Value *, 4> idx) {
  llvm::IRBuilder<> builder(BB);
  llvm::Value *idx_val = this->AtVal_(BB, idx);

  return builder.CreateInBoundsGEP(llvm_buffer_, idx_val);
}

llvm::Value *Hobbit::Tensor::GEP(llvm::BasicBlock *BB, uint64_t raw_idx) {
  llvm::IRBuilder<> builder(BB);
  llvm::Value *idx_val = builder.getInt64(raw_idx);

  return builder.CreateInBoundsGEP(llvm_buffer_, idx_val);
}

llvm::Value *Hobbit::Tensor::GEP(llvm::BasicBlock *BB, llvm::Value *raw_idx) {
  llvm::IRBuilder<> builder(BB);

  return builder.CreateInBoundsGEP(llvm_buffer_, raw_idx);
}

llvm::Value *Hobbit::Tensor::Load(llvm::BasicBlock *BB,
                                  llvm::SmallVector<uint64_t, 4> idx) {
  llvm::Value *gep = this->GEP(BB, std::move(idx));
  llvm::IRBuilder<> builder(BB);

  return builder.CreateAlignedLoad(gep, ALIGNMENT);
}

llvm::Value *Hobbit::Tensor::Load(llvm::BasicBlock *BB,
                                  llvm::SmallVector<llvm::Value *, 4> idx) {
  llvm::Value *gep = this->GEP(BB, idx);
  llvm::IRBuilder<> builder(BB);

  return builder.CreateAlignedLoad(gep, ALIGNMENT);
}

llvm::Value *Hobbit::Tensor::Load(llvm::BasicBlock *BB, uint64_t raw_idx) {
  llvm::Value *gep = this->GEP(BB, raw_idx);
  llvm::IRBuilder<> builder(BB);

  return builder.CreateAlignedLoad(gep, ALIGNMENT);
}

llvm::Value *Hobbit::Tensor::Load(llvm::BasicBlock *BB, llvm::Value *raw_idx) {
  llvm::Value *gep = this->GEP(BB, raw_idx);
  llvm::IRBuilder<> builder(BB);

  return builder.CreateAlignedLoad(gep, ALIGNMENT);
}

void Hobbit::Tensor::Store(llvm::BasicBlock *BB,
                           llvm::SmallVector<uint64_t, 4> idx,
                           llvm::Value *val) {
  llvm::Value *gep = this->GEP(BB, std::move(idx));
  llvm::IRBuilder<> builder(BB);

  builder.CreateAlignedStore(val, gep, ALIGNMENT);
}

void Hobbit::Tensor::Store(llvm::BasicBlock *BB,
                           llvm::SmallVector<llvm::Value *, 4> idx,
                           llvm::Value *val) {
  llvm::Value *gep = this->GEP(BB, std::move(idx));
  llvm::IRBuilder<> builder(BB);

  builder.CreateAlignedStore(val, gep, ALIGNMENT);
}

void Hobbit::Tensor::Store(llvm::BasicBlock *BB, uint64_t raw_idx,
                           llvm::Value *val) {
  llvm::Value *gep = this->GEP(BB, std::move(raw_idx));
  llvm::IRBuilder<> builder(BB);

  builder.CreateAlignedStore(val, gep, ALIGNMENT);
}

void Hobbit::Tensor::Store(llvm::BasicBlock *BB, llvm::Value *raw_idx,
                           llvm::Value *val) {
  llvm::Value *gep = this->GEP(BB, std::move(raw_idx));
  llvm::IRBuilder<> builder(BB);

  builder.CreateAlignedStore(val, gep, ALIGNMENT);
}

Hobbit::Tensor::Tensor(llvm::Value *ptr, llvm::SmallVector<uint64_t, 4> dims,
                       llvm::Type *type)
    : llvm_buffer_(ptr), dims_(std::move(dims)), llvm_type_(type),
      Node(nullptr, nullptr) {}

uint64_t Hobbit::Tensor::At_(llvm::SmallVector<uint64_t, 4> idx) {
  if (idx.size() != dims_.size())
    LOG(FATAL) << "Incorrect number of indices";

  uint64_t out = 0;
  for (uint64_t i = 0; i < dims_.size() - 1; i++) {
    out += idx[i];
    out *= dims_[i + 1];
  }
  out += *idx.end();

  return out;
}

llvm::Value *Hobbit::Tensor::AtVal_(llvm::BasicBlock *BB,
                                    llvm::SmallVector<llvm::Value *, 4> idx) {
  llvm::IRBuilder<> builder(BB);
  llvm::Value *out = builder.getInt64(0);

  for (uint64_t i = 0; i < dims_.size() - 1; i++) {
    out = builder.CreateAdd(out, idx[i]);
    out = builder.CreateMul(out, builder.getInt64(dims_[i + 1]));
  }
  out = builder.CreateAdd(out, *idx.end());

  return out;
}
