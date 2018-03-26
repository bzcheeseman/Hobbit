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
#include <utility>

#include <glog/logging.h>

#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>

Hobbit::ast::Function *Hobbit::ast::Function::Create(const std::string &name) {
  Function *f = new Function;
  f->name_ = "hobbit."+name;

  return f;
}

const std::string &Hobbit::ast::Function::GetName() { return name_; }

Hobbit::ast::Tensor *
Hobbit::ast::Function::GetNewArg(const std::string &name,
                                 llvm::SmallVector<uint64_t, 4> dims,
                                 llvm::Type *type) {
  // Create a new Tensor
  Tensor *arg = Tensor::CreateVariable(name, this, std::move(dims), type);
  // Add the tensor to the arg table
  arg_table_.push_back(arg);
  return arg;
}

void Hobbit::ast::Function::SetArg(Hobbit::ast::Tensor *t) {
  arg_table_.push_back(t);
}

void Hobbit::ast::Function::PushNode(Hobbit::ast::Node *node) {
  this->op_table_.push_back(node);
}

void Hobbit::ast::Function::Emit(Hobbit::ast::Visitor *CG) {
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
  llvm::BasicBlock::Create(*ctx, name_ + ".entry", out);

  llvm::Function::arg_iterator iter = out->arg_begin();
  for (auto &arg : arg_table_) {
    arg->SetBuffer(&(*iter++));
  }

  CG->PushFunction(this, out); // keyed on AST function

  for (auto &op : op_table_) {
    op->Emit(CG);
  }

  out = CG->GetFunction(this);

  // Create a return for the exit BB
  llvm::BasicBlock *exit_bb = &*(--out->end());
  llvm::IRBuilder<> builder(exit_bb);
  builder.CreateRetVoid();

  // Finalize the entry BB if that hasn't already happened
  llvm::BasicBlock *entry_bb = &out->getEntryBlock();
  llvm::BranchInst *br;
//  bool has_branch = false;
//  bool branch_is_last = false;
  for (auto &instr : *entry_bb) {
    br = llvm::dyn_cast<llvm::BranchInst>(&instr);
    if (br) break;
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

// llvm::Function *Hobbit::ast::Function::EmitFunction(llvm::Module *module) {
//  llvm::LLVMContext &ctx = module->getContext();
//
//  llvm::SmallVector<llvm::Type *, 4> arg_types;
//  for (auto &arg : arg_table_) {
//    arg_types.push_back(arg->GetType());
//  }
//
//  llvm::FunctionType *ft =
//          llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), arg_types,
//          false);
//  llvm::Function *out =
//          llvm::cast<llvm::Function>(module->getOrInsertFunction(name_, ft));
//  llvm::BasicBlock *entryBB = llvm::BasicBlock::Create(
//          ctx, "hobbit." + name_ + ".entry", out);
//
//  // Set up the args properly - modify the pointers that are hanging around
//  everywhere
//  llvm::Function::arg_iterator iter = out->arg_begin();
//  for (auto &arg : arg_table_) {
//    arg->SetBuffer(&(*iter++));
//  }
//
//  llvm::BasicBlock *bb = entryBB;
//  for (auto &op : op_table_) {
//    bb = op->Emit(bb);
//  }
//
//  // Take care of the end
//  last_bb = std::move(bb);
//  last_bb->setName("hobbit." + name_ + ".exit");
//  llvm::IRBuilder<> builder(last_bb);
//  builder.CreateRetVoid();
//
//  return out;
//}

Hobbit::ast::Loop::Loop(const std::string &name, Hobbit::ast::Node *parent,
                        uint64_t range_start, uint64_t range_end)
    : name_(name), parent_(parent), range_start_(range_start),
      range_end_(range_end) {}

llvm::PHINode *Hobbit::ast::Loop::AddLoopEntry_(llvm::BasicBlock *prev_bb) {
  llvm::LLVMContext &ctx = prev_bb->getContext();
  llvm::Function *parent_func = prev_bb->getParent();

  llvm::BasicBlock *loop_body =
      llvm::BasicBlock::Create(ctx, this->name_ + ".body", parent_func);

  llvm::IRBuilder<> builder(prev_bb);
  builder.CreateBr(loop_body);

  builder.SetInsertPoint(loop_body);
  llvm::PHINode *index_var =
      builder.CreatePHI(builder.getInt64Ty(), 2, this->name_ + ".idx");
  index_var->addIncoming(builder.getInt64(range_start_), prev_bb);

  return index_var;
}

llvm::BasicBlock *Hobbit::ast::Loop::AddLoopExit_(llvm::PHINode *idx_var) {
  llvm::BasicBlock *loop_body = idx_var->getParent();
  llvm::LLVMContext &ctx = idx_var->getContext();
  llvm::Function *parent_func = idx_var->getFunction();

  llvm::BasicBlock *loop_exit =
      llvm::BasicBlock::Create(ctx, this->name_ + ".exit", parent_func);

  llvm::IRBuilder<> builder(loop_body);

  llvm::Value *next_idx_var = builder.CreateAdd(idx_var, builder.getInt64(1));
  llvm::Value *end_cond =
      builder.CreateICmpEQ(next_idx_var, builder.getInt64(range_end_));
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
      llvm::ConstantAsMetadata::get(
          llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx), 8))};
  args.push_back(llvm::MDNode::get(ctx, vecMD));

  llvm::MDNode *LoopID = llvm::MDNode::get(ctx, args);
  LoopID->replaceOperandWith(0, LoopID);

  loop_end_br->setMetadata("llvm.loop", LoopID);
}

Hobbit::ast::HSum *Hobbit::ast::HSum::Create(const std::string &name,
                                             Hobbit::ast::Node *parent,
                                             uint64_t range_start,
                                             uint64_t range_end) {
  HSum *out = new HSum("hobbit."+name, parent, range_start, range_end);
  return out;
}

Hobbit::ast::HSum::HSum(const std::string &name, Hobbit::ast::Node *parent,
                        uint64_t range_start, uint64_t range_end)
    : Loop(name, parent, range_start, range_end) {
  ;
}

void Hobbit::ast::HSum::AddKernel(
    llvm::SmallVector<llvm::PHINode *, 4> idx_vars) {
  llvm::PHINode *idx = idx_vars[0];
  llvm::LLVMContext &ctx = idx->getContext();
  llvm::BasicBlock *bb = idx->getParent();

  Tensor *arg = args_[0];

  llvm::Type *accumulator_type = arg->GetType();
  if (accumulator_type->isPointerTy()) {
    accumulator_type = accumulator_type->getPointerElementType();
  }

  llvm::IRBuilder<> builder(&bb->getParent()->getEntryBlock());

  llvm::Value *zero =
          builder.CreateBitCast(builder.getInt64(0), accumulator_type);

  out_[0]->Store(builder.GetInsertBlock(), (uint64_t)0, zero);

  builder.SetInsertPoint(bb);

  //  llvm::PHINode *accumulator = builder.CreatePHI(accumulator_type, 2, name_
  //  + ".accumulator");
  //  accumulator->addIncoming(zero, bb->getSinglePredecessor());

  llvm::Value *accumulator_next;
  if (accumulator_type->isIntegerTy()) {
    accumulator_next =
        builder.CreateAdd(out_[0]->Load(bb, (uint64_t)0), arg->Load(bb, idx));
  }
  if (accumulator_type->isFloatingPointTy()) {
    accumulator_next =
        builder.CreateFAdd(out_[0]->Load(bb, (uint64_t)0), arg->Load(bb, idx));
  }

  out_[0]->Store(bb, (uint64_t)0, accumulator_next);

  //  accumulator->addIncoming(accumulator_next, bb);
}

const std::string &Hobbit::ast::HSum::GetName() { return name_; }

Hobbit::ast::Tensor *
Hobbit::ast::HSum::SetArgs(llvm::SmallVector<Tensor *, 2> args) {
  if (args.size() != 1)
    LOG(FATAL) << "Too many arguments passed to " + name_;
  if (args[0]->NDim() != 1)
    LOG(WARNING) << "Treating input tensor as flat within " + name_;

  args_ = std::move(args);
  args_[0] = args_[0]->Flatten();

  uint64_t range = range_end_ - range_start_;
  if (args_[0]->Dim(0) != range)
    LOG(FATAL) << "Incorrect size passed in " + name_;
  dim_idx_ = 0;

  //  Tensor *output =
  //  llvm::dyn_cast<Function>(parent_)->GetNewArg(name_+".output", {1},
  //  args_[0]->GetType());

  out_.push_back(Tensor::CreateVariable(name_ + ".output", this, {1},
                                        args_[0]->GetType()));

  return out_[0];
}

void Hobbit::ast::HSum::Emit(Hobbit::ast::Visitor *CG) {
  llvm::Function *f;
  Node *n = parent_;
  Function *parent;
  while (!(parent = llvm::dyn_cast<Function>(n))) {
    n = n->GetParent();
  }
  f = CG->GetFunction(parent); // takes the parent function of this node

  llvm::BasicBlock *prev_bb = &(*(--f->end()));
  llvm::PHINode *idx_var = this->AddLoopEntry_(prev_bb);
  this->AddKernel({idx_var});
  this->AddLoopExit_(idx_var);
}

// llvm::BasicBlock *Hobbit::ast::HSum::Emit(llvm::BasicBlock *prev_bb)  {
//  llvm::PHINode *idx_var = this->AddLoopEntry_(prev_bb);
//  this->AddKernel({idx_var});
//  return this->AddLoopExit_(idx_var);
//}

Hobbit::ast::Tensor *Hobbit::ast::Tensor::CreateVariable(
    const std::string &name, Hobbit::ast::Node *parent,
    llvm::SmallVector<uint64_t, 4> dims, llvm::Type *type) {
  Tensor *t = new Tensor(nullptr, std::move(dims), type);
  t->name_ = name;
  t->parent_ = parent;

  return t;
}

Hobbit::ast::Tensor *
Hobbit::ast::Tensor::CreateConstant(const std::string &name, Node *parent,
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
        buffer_constants.push_back(llvm::ConstantFP::get(type, (double)buf[i]));
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

  // TODO: Add a pass that will put an alloca for this into the entry block
  //  %3 = getelementptr inbounds [6 x float], [6 x float]* %0, i64 0, i64 0
  //  %4 = getelementptr inbounds [6 x float], [6 x float]* %1, i64 0, i64 0
  t->llvm_buffer_ = llvm::ConstantArray::get(arr_type, buffer_constants);
  t->llvm_type_ = t->llvm_buffer_->getType();
  buffer_constants.clear();

  return t;
}

Hobbit::ast::Node *Hobbit::ast::Tensor::GetParent() { return parent_; }

llvm::Type *Hobbit::ast::Tensor::GetType() { return llvm_type_; }

void Hobbit::ast::Tensor::SetBuffer(llvm::Value *val) {
  this->llvm_buffer_ = val;
  this->llvm_type_ = val->getType();

  // set the name of the buffer
  this->llvm_buffer_->setName(this->name_);
}

uint64_t Hobbit::ast::Tensor::NDim() { return dims_.size(); }

uint64_t Hobbit::ast::Tensor::Dim(uint64_t which) { return dims_[which]; }

uint64_t Hobbit::ast::Tensor::Size() {
  uint64_t total_size = 1;
  for (auto &dim : dims_) {
    total_size *= dim;
  }
  return total_size;
}

Hobbit::ast::Tensor *
Hobbit::ast::Tensor::Chip(llvm::BasicBlock *BB,
                          llvm::SmallVector<uint64_t, 4> start_idx,
                          llvm::SmallVector<uint64_t, 4> dims) {
  // create a new tensor that aliases this
  Tensor *chip = new Tensor(this->GEP(BB, std::move(start_idx)),
                            std::move(dims), this->llvm_type_);
  chip->parent_ = this;

  return chip;
}

Hobbit::ast::Tensor *
Hobbit::ast::Tensor::Chip(llvm::BasicBlock *BB,
                          llvm::SmallVector<llvm::Value *, 4> start_idx,
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

llvm::Value *Hobbit::ast::Tensor::GEP(llvm::BasicBlock *BB,
                                      llvm::SmallVector<uint64_t, 4> idx) {
  llvm::IRBuilder<> builder(BB);
  llvm::Value *idx_val = builder.getInt64(this->At_(std::move(idx)));

  if (llvm_type_->isArrayTy()) {
    return builder.CreateInBoundsGEP(llvm_buffer_, {builder.getInt64(0), idx_val});
  }

  return builder.CreateInBoundsGEP(llvm_buffer_, idx_val);
}

llvm::Value *Hobbit::ast::Tensor::GEP(llvm::BasicBlock *BB,
                                      llvm::SmallVector<llvm::Value *, 4> idx) {
  llvm::IRBuilder<> builder(BB);
  llvm::Value *idx_val = this->AtVal_(BB, idx);

  if (llvm_type_->isArrayTy()) {
    return builder.CreateInBoundsGEP(llvm_buffer_, {builder.getInt64(0), idx_val});
  }

  return builder.CreateInBoundsGEP(llvm_buffer_, idx_val);
}

llvm::Value *Hobbit::ast::Tensor::GEP(llvm::BasicBlock *BB, uint64_t raw_idx) {
  llvm::IRBuilder<> builder(BB);
  llvm::Value *idx_val = builder.getInt64(raw_idx);

  if (llvm_type_->isArrayTy()) {
    return builder.CreateInBoundsGEP(llvm_buffer_, {builder.getInt64(0), idx_val});
  }

  return builder.CreateInBoundsGEP(llvm_buffer_, idx_val);
}

llvm::Value *Hobbit::ast::Tensor::GEP(llvm::BasicBlock *BB,
                                      llvm::Value *raw_idx) {
  llvm::IRBuilder<> builder(BB);

  if (llvm_type_->isArrayTy()) {
    return builder.CreateInBoundsGEP(llvm_buffer_, {builder.getInt64(0), raw_idx});
  }

  return builder.CreateInBoundsGEP(llvm_buffer_, raw_idx);
}

llvm::Value *Hobbit::ast::Tensor::Load(llvm::BasicBlock *BB,
                                       llvm::SmallVector<uint64_t, 4> idx) {
  llvm::Value *gep = this->GEP(BB, std::move(idx));
  llvm::IRBuilder<> builder(BB);

  return builder.CreateAlignedLoad(gep, ALIGNMENT);
}

llvm::Value *
Hobbit::ast::Tensor::Load(llvm::BasicBlock *BB,
                          llvm::SmallVector<llvm::Value *, 4> idx) {
  llvm::Value *gep = this->GEP(BB, idx);
  llvm::IRBuilder<> builder(BB);

  return builder.CreateAlignedLoad(gep, ALIGNMENT);
}

llvm::Value *Hobbit::ast::Tensor::Load(llvm::BasicBlock *BB, uint64_t raw_idx) {
  llvm::Value *gep = this->GEP(BB, raw_idx);
  llvm::IRBuilder<> builder(BB);

  return builder.CreateAlignedLoad(gep, ALIGNMENT);
}

llvm::Value *Hobbit::ast::Tensor::Load(llvm::BasicBlock *BB,
                                       llvm::Value *raw_idx) {
  llvm::Value *gep = this->GEP(BB, raw_idx);
  llvm::IRBuilder<> builder(BB);

  return builder.CreateAlignedLoad(gep, ALIGNMENT);
}

void Hobbit::ast::Tensor::Store(llvm::BasicBlock *BB,
                                llvm::SmallVector<uint64_t, 4> idx,
                                llvm::Value *val) {
  llvm::Value *gep = this->GEP(BB, std::move(idx));
  llvm::IRBuilder<> builder(BB);

  builder.CreateAlignedStore(val, gep, ALIGNMENT);
}

void Hobbit::ast::Tensor::Store(llvm::BasicBlock *BB,
                                llvm::SmallVector<llvm::Value *, 4> idx,
                                llvm::Value *val) {
  llvm::Value *gep = this->GEP(BB, std::move(idx));
  llvm::IRBuilder<> builder(BB);

  builder.CreateAlignedStore(val, gep, ALIGNMENT);
}

void Hobbit::ast::Tensor::Store(llvm::BasicBlock *BB, uint64_t raw_idx,
                                llvm::Value *val) {
  llvm::Value *gep = this->GEP(BB, std::move(raw_idx));
  llvm::IRBuilder<> builder(BB);

  builder.CreateAlignedStore(val, gep, ALIGNMENT);
}

void Hobbit::ast::Tensor::Store(llvm::BasicBlock *BB, llvm::Value *raw_idx,
                                llvm::Value *val) {
  llvm::Value *gep = this->GEP(BB, std::move(raw_idx));
  llvm::IRBuilder<> builder(BB);

  builder.CreateAlignedStore(val, gep, ALIGNMENT);
}

Hobbit::ast::Tensor::Tensor(llvm::Value *ptr,
                            llvm::SmallVector<uint64_t, 4> dims,
                            llvm::Type *type)
    : llvm_buffer_(ptr), dims_(std::move(dims)), llvm_type_(type) {}

uint64_t Hobbit::ast::Tensor::At_(llvm::SmallVector<uint64_t, 4> idx) {
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

llvm::Value *
Hobbit::ast::Tensor::AtVal_(llvm::BasicBlock *BB,
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

const std::string &Hobbit::ast::Tensor::GetName() { return name_; }