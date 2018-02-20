//
// Created by Aman on 2/20/18.
//

#include "Variable.hpp"

Hobbit::Variable::Variable(llvm::Value *value, uint64_t &ptr_size) {
  m_value_ = value;
  m_type_ = value->getType();
  ptr_array_size_ = ptr_size;
}

llvm::ArrayRef<llvm::Value *> Hobbit::Variable::Pack(llvm::IRBuilder<> &builder, uint32_t chunk_size) {
  llvm::Type *vector_type = llvm::VectorType::get(m_type_, chunk_size);

  llvm::ArrayRef<llvm::Value *> chunks;
  if (m_type_->isPointerTy()) {
    assert(ptr_array_size_ != 0);
    return PackPtrToVector_(builder, vector_type);
  }
  if (m_type_->isArrayTy()) {
    return PackArrayToVector_(builder, vector_type);
  }

  return {nullptr};
}

llvm::ArrayRef<Hobbit::Variable> Hobbit::Variable::Split(llvm::IRBuilder<> &builder, uint32_t chunk_size) {
  llvm::Type *m_type = llvm::ArrayType::get(m_type_, chunk_size);

  if (m_type_->isPointerTy()) {
    assert(ptr_array_size_ != 0);
    return PackPtrToVariable_(builder, m_type);
  }
  if (m_type_->isArrayTy()) {
    return PackArrayToVariable_(builder, m_type);
  }

  return {nullptr};

}

llvm::ArrayRef<llvm::Value *> Hobbit::Variable::PackPtrToVector_(llvm::IRBuilder<> &builder, llvm::Type *vector_type) {
  uint64_t vector_elements = vector_type->getVectorNumElements();
  uint64_t leftovers = ptr_array_size_ % vector_elements;
  uint64_t num_vectors = (ptr_array_size_ - leftovers)/vector_elements;

  std::vector<llvm::Value *> output;

  for (uint64_t i = 0; i < num_vectors; i++) {
    llvm::Value *tmp = llvm::UndefValue::get(vector_type);
    for (uint64_t j = 0; j < vector_elements; j++) {
      llvm::Value *ptr_elem = builder.CreateLoad(
              builder.CreateGEP(m_value_, builder.getInt64(i*vector_elements + j))
      );
      builder.CreateInsertElement(tmp, ptr_elem, j);
    }
    output.push_back(tmp);
  }

  if (leftovers != 0) { // add the last few to the vector
    llvm::Value *tmp = llvm::UndefValue::get(vector_type);
    for (uint64_t j = 0; j < leftovers; j++) {
      llvm::Value *ptr_elem = builder.CreateLoad(
              builder.CreateGEP(m_value_, builder.getInt64(num_vectors*vector_elements + j))
      );
      builder.CreateInsertElement(tmp, ptr_elem, j, m_value_->getName() + ".pack");
    }
    output.push_back(tmp);
  }

  return output;
}

llvm::ArrayRef<llvm::Value *>
Hobbit::Variable::PackArrayToVector_(llvm::IRBuilder<> &builder, llvm::Type *vector_type) {
  uint64_t vector_elements = vector_type->getVectorNumElements();
  uint64_t array_num_elements = m_type_->getArrayNumElements();
  uint64_t leftovers = array_num_elements % vector_elements;
  uint64_t num_vectors = (array_num_elements - leftovers)/vector_elements;

  std::vector<llvm::Value *> output;

  for (uint64_t i = 0; i < num_vectors; i++) {
    llvm::Value *tmp = llvm::UndefValue::get(vector_type);
    for (uint64_t j = 0; j < vector_elements; j++) {
      llvm::Value *ptr_elem = builder.CreateLoad(
              builder.CreateGEP(m_value_, builder.getInt64(i*vector_elements + j))
      );
      builder.CreateInsertElement(tmp, ptr_elem, j);
    }
    output.push_back(tmp);
  }

  if (leftovers != 0) { // add the last few to the vector
    llvm::Value *tmp = llvm::UndefValue::get(vector_type);
    for (uint64_t j = 0; j < leftovers; j++) {
      llvm::Value *ptr_elem = builder.CreateLoad(
              builder.CreateGEP(m_value_, builder.getInt64(num_vectors*vector_elements + j))
      );
      builder.CreateInsertElement(tmp, ptr_elem, j, m_value_->getName() + ".pack");
    }
    output.push_back(tmp);
  }

  return output;
}

llvm::ArrayRef<Hobbit::Variable>
Hobbit::Variable::PackPtrToVariable_(llvm::IRBuilder<> &builder, llvm::Type *array_type) {
  uint64_t array_elements = array_type->getArrayNumElements();
  uint64_t leftovers = ptr_array_size_ % array_elements;
  uint64_t num_vectors = (ptr_array_size_ - leftovers)/array_elements;

  std::vector<Variable> output;

  for (uint64_t i = 0; i < num_vectors; i++) {
    llvm::Value *tmp = llvm::UndefValue::get(array_type);
    for (uint64_t j = 0; j < array_elements; j++) {
      llvm::Value *ptr_elem = builder.CreateLoad(
              builder.CreateGEP(m_value_, builder.getInt64(i*array_elements + j))
      );
      builder.CreateInsertElement(tmp, ptr_elem, j);
    }
    output.emplace_back(tmp, 0);
  }

  if (leftovers != 0) { // add the last few to the vector
    llvm::Value *tmp = llvm::UndefValue::get(array_type);
    for (uint64_t j = 0; j < leftovers; j++) {
      llvm::Value *ptr_elem = builder.CreateLoad(
              builder.CreateGEP(m_value_, builder.getInt64(num_vectors*array_elements + j))
      );
      builder.CreateInsertElement(tmp, ptr_elem, j, m_value_->getName() + ".pack");
    }
    output.emplace_back(tmp, 0);
  }

  return output;
}

llvm::ArrayRef<Hobbit::Variable>
Hobbit::Variable::PackArrayToVariable_(llvm::IRBuilder<> &builder, llvm::Type *array_type) {
  uint64_t array_elements = array_type->getArrayNumElements();
  uint64_t array_num_elements = m_type_->getArrayNumElements();
  uint64_t leftovers = array_num_elements % array_elements;
  uint64_t num_vectors = (array_num_elements - leftovers)/array_elements;

  std::vector<Variable> output;

  for (uint64_t i = 0; i < num_vectors; i++) {
    llvm::Value *tmp = llvm::UndefValue::get(array_type);
    for (uint64_t j = 0; j < array_elements; j++) {
      llvm::Value *ptr_elem = builder.CreateLoad(
              builder.CreateGEP(m_value_, builder.getInt64(i*array_elements + j))
      );
      builder.CreateInsertElement(tmp, ptr_elem, j);
    }
    output.emplace_back(tmp, 0);
  }

  if (leftovers != 0) { // add the last few to the vector
    llvm::Value *tmp = llvm::UndefValue::get(array_type);
    for (uint64_t j = 0; j < leftovers; j++) {
      llvm::Value *ptr_elem = builder.CreateLoad(
              builder.CreateGEP(m_value_, builder.getInt64(num_vectors*array_elements + j))
      );
      builder.CreateInsertElement(tmp, ptr_elem, j, m_value_->getName() + ".pack");
    }
    output.emplace_back(tmp, 0);
  }

  return output;
}

Hobbit::Constant::Constant(llvm::Value *value, uint64_t &ptr_size) : Variable(value, ptr_size) {
  ;
}
