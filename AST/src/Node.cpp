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
    // TODO: Store a zero in arg
  }

  for (auto &alloca : alloca_table_) {
    llvm::Type *alloca_type = alloca->GetType();
    if (alloca_type->isPointerTy())
      alloca_type = alloca_type->getPointerElementType();
    alloca_type = llvm::ArrayType::get(alloca_type, alloca->Size());
    llvm::Value *a = builder.CreateAlloca(alloca_type);
    builder.CreateStore(
            llvm::ConstantAggregateZero::get(alloca_type), a
    );
    llvm::Value *ptr = builder.CreateInBoundsGEP(a, {builder.getInt64(0), builder.getInt64(0)});
    alloca->SetBuffer(ptr);
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
  if (entry_bb->empty()) {
    llvm::outs() << "Entry BB empty\n";
    builder.SetInsertPoint(entry_bb);
    builder.CreateBr(&*(++out->begin()));
  }

  llvm::BranchInst *br;
  for (auto &instr : *entry_bb) {
    br = llvm::dyn_cast<llvm::BranchInst>(&instr);
    if (br)
      break;
  }
  if (!br) {
    llvm::outs() << "No branch inst found\n";
    builder.SetInsertPoint(entry_bb);
    builder.CreateBr(&*(++out->begin()));
  }
  llvm::outs() << "Branch inst found\n";
  br->moveAfter(&(*(--entry_bb->end())));
  return;

}

Hobbit::Loop::Loop(const std::string &name, Hobbit::ast::Node *parent,
                   uint64_t range_start, uint64_t range_end)
    : name_(name), Node(parent, nullptr) {
  loop_info_.start = range_start;
  loop_info_.end = range_end;
  inner_ = nullptr;
  outer_ = nullptr;
}

void Hobbit::Loop::Emit(Hobbit::Visitor *CG) {
  llvm::Function *f;
  Node *n = parent_;
  Function *parent;
  while (!(parent = llvm::dyn_cast<Function>(n))) {
    n = n->GetParent();
  }
  f = CG->GetFunction(parent); // takes the parent function of this node

  llvm::BasicBlock *pre_loop = &*--f->end();

  llvm::IRBuilder<> builder(pre_loop);

  llvm::BasicBlock *loop_entry =
          llvm::BasicBlock::Create(f->getContext(), name_ + ".loop.entry", f);
  builder.CreateBr(loop_entry);

  llvm::BasicBlock *body_bb = this->AddLoopEntry_(loop_entry);
  this->AddKernel_(body_bb);
  llvm::BasicBlock *after_exit = this->AddLoopExit_(body_bb);
}

llvm::BasicBlock *Hobbit::Loop::AddLoopEntry_(llvm::BasicBlock *phi_bb) {
  llvm::LLVMContext &ctx = phi_bb->getContext();
  llvm::Function *parent_func = phi_bb->getParent();

  llvm::BasicBlock *entry = phi_bb->getSinglePredecessor();

  llvm::IRBuilder<> builder(phi_bb);

  loop_info_.phi = builder.CreatePHI(builder.getInt64Ty(), 2, name_ + ".idx");
  loop_info_.phi->addIncoming(builder.getInt64(loop_info_.start), entry);
  loop_info_.increment = builder.getInt64(1);
  loop_info_.cumulative_idx = loop_info_.phi;

  llvm::BasicBlock *loop_body = llvm::BasicBlock::Create(ctx, name_ + ".loop.body", parent_func);
  builder.CreateBr(loop_body);
  builder.SetInsertPoint(loop_body);
  return loop_body;
}

llvm::BasicBlock *Hobbit::Loop::AddLoopExit_(llvm::BasicBlock *body_bb) {
  llvm::PHINode *phi = loop_info_.phi;
  llvm::BasicBlock *phi_block = phi->getParent();
  llvm::LLVMContext &ctx = phi->getContext();
  llvm::Function *parent_func = phi->getFunction();

  llvm::BasicBlock *exit_bb = llvm::BasicBlock::Create(ctx, this->name_ + ".loop.exit", parent_func);
  llvm::BasicBlock *after_exit = llvm::BasicBlock::Create(ctx, this->name_ + ".exit", parent_func);

  llvm::IRBuilder<> builder(body_bb);
  builder.CreateBr(exit_bb);
  builder.SetInsertPoint(exit_bb);

  llvm::Value *next_idx_var = builder.CreateAdd(phi, loop_info_.increment);
  llvm::Value *end_cond =
          builder.CreateICmpEQ(next_idx_var, builder.getInt64(loop_info_.end));
  llvm::BranchInst *br = builder.CreateCondBr(end_cond, after_exit, phi_block);
  this->AddLoopMetadata_(br);
  phi->addIncoming(next_idx_var, exit_bb);

  builder.SetInsertPoint(after_exit);

  return after_exit;
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

Hobbit::Tensor *Hobbit::HSum::SetArgs(llvm::SmallVector<Tensor *, 2> args) {
  if (args.size() != 1)
    LOG(FATAL) << "Too many arguments passed to " + name_;
  if (args[0]->NDim() != 1)
    LOG(WARNING) << "Treating input tensor as flat within " + name_;

  args_ = std::move(args);
  args_[0] = args_[0]->Flatten();

  uint64_t range = loop_info_.end - loop_info_.start;
  if (args_[0]->Dim(0) != range)
    LOG(FATAL) << "Incorrect size passed in " + name_;

  llvm::Type *output_type = args_[0]->GetType();
  if (output_type->isArrayTy()) {
    output_type = output_type->getArrayElementType()->getPointerTo(0);
  }

  Tensor *output = llvm::dyn_cast<Function>(parent_)->GetNewAlloca(
          name_ + ".output", {1}, output_type);

  out_.push_back(std::move(output));

  return out_[0];
}

void Hobbit::HSum::AddKernel_(llvm::BasicBlock *body_bb) {
  llvm::LLVMContext &ctx = body_bb->getContext();

  Tensor *arg = args_[0];

  llvm::Type *accumulator_type = arg->GetType();
  if (accumulator_type->isPointerTy()) {
    accumulator_type = accumulator_type->getPointerElementType();
  }
  if (accumulator_type->isArrayTy()) {
    accumulator_type = accumulator_type->getArrayElementType();
  }

  llvm::IRBuilder<> builder(&body_bb->getParent()->getEntryBlock());

  builder.SetInsertPoint(body_bb);

  llvm::Value *accumulator_next;
  if (accumulator_type->isIntegerTy()) {
    accumulator_next =
            builder.CreateAdd(
                    out_[0]->Load(body_bb, (uint64_t)0),
                    arg->Load(body_bb, this->loop_info_.cumulative_idx)
            );
  }
  if (accumulator_type->isFloatingPointTy()) {
    accumulator_next =
            builder.CreateFAdd(
                    out_[0]->Load(body_bb, (uint64_t)0),
                    arg->Load(body_bb, this->loop_info_.cumulative_idx)
            );
  }

  out_[0]->Store(body_bb, (uint64_t)0, accumulator_next);
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
