#include "Variable.hpp"

Hobbit::Shape::Shape(const uint32_t &k, const uint32_t &h, const uint32_t &w) : k_(k), h_(h), w_(w) {}

uint32_t Hobbit::Shape::At(const uint32_t &k, const uint32_t &h, const uint32_t &w) {
  if (k >= k_) throw std::runtime_error("Invalid index, k = " + std::to_string(k) + "geq k_ = " + std::to_string(k_));
  if (h >= h_) throw std::runtime_error("Invalid index, h = " + std::to_string(h) + "geq h_ = " + std::to_string(h_));
  if (w >= w_) throw std::runtime_error("Invalid index, w = " + std::to_string(w) + "geq w_ = " + std::to_string(w_));

  return (k*h_ + h)*w_ + w;
}

Hobbit::Shape
Hobbit::Shape::GetChunkShape(const Hobbit::Range &k_range, const Hobbit::Range &h_range, const Hobbit::Range &w_range) {
  return {k_range.end - k_range.start, h_range.end - h_range.start, w_range.end - w_range.start};
}

Hobbit::Range
Hobbit::Shape::GetChunkIdx(const Hobbit::Range &k_range, const Hobbit::Range &h_range, const Hobbit::Range &w_range) {
  Range idx_range(0, k_*h_*w_);
  uint32_t k_start, k_end, h_start, h_end, w_start, w_end;

  if (k_range.start == 0 && k_range.end == 0) {
    k_start = 0;
    k_end = k_ - 1;
  }
  else {
    k_start = k_range.start;
    k_end = k_range.end - 1; // so if the range is {0, 1} we just have a single k index
  }

  if (h_range.start == 0 && h_range.end == 0) {
    h_start = 0;
    h_end = h_ - 1;
  }
  else {
    h_start = h_range.start;
    h_end = h_range.end - 1;
  }

  if (w_range.start == 0 && w_range.end == 0) {
    w_start = 0;
    w_end = w_ - 1;
  }
  else {
    w_start = w_range.start;
    w_end = w_range.end - 1;
  }

  idx_range.start = At(k_start, h_start, w_start);
  idx_range.end = At(k_end, h_end, w_end);

  return idx_range;
}

uint32_t Hobbit::Shape::GetSize() {
  return k_*h_*w_;
}

const uint32_t &Hobbit::Shape::GetAxisSize(Hobbit::Axis &axis) {
  switch (axis) {
    case K: return k_;
    case H: return h_;
    case W: return w_;
  }

  return 0;
}

Hobbit::Variable::Variable(llvm::Type *scalar_type, const Hobbit::Shape &shape) : m_shape_(shape) {
  m_type_ = llvm::ArrayType::get(scalar_type, m_shape_.GetSize());
  m_value_ = new llvm::AllocaInst(m_type_);
}

Hobbit::Variable::Variable(llvm::Value *value, llvm::Type *type, const Hobbit::Shape &shape, Variable *parent) :
        m_shape_(shape), m_value_(value), m_type_(type), m_parent_(parent) {
  ;
}

Hobbit::Variable::~Variable() {
  // If I have a parent, remove myself from their list
  if (m_parent_ != nullptr) {
    m_parent_->m_children_.remove(this); // remove myself from their list
  }

  // If I have children, delete them
  if (!m_children_.empty()) {
    for (auto &child : m_children_) {
      delete child;
    }
  }
}

Hobbit::Variable *
Hobbit::Variable::GetChunk(llvm::IRBuilder<> &builder, const Hobbit::Range &k_range, const Hobbit::Range &h_range,
                           const Hobbit::Range &w_range) {
  Range idx_range = m_shape_.GetChunkIdx(k_range, h_range, w_range);
  Shape chip_shape = m_shape_.GetChunkShape(k_range, h_range, w_range);

  llvm::Value *chip_start_ptr = builder.CreateGEP(m_value_, builder.getInt32(idx_range.start));

  Variable *output = new Variable (chip_start_ptr, m_type_->getArrayElementType(), chip_shape, this);

  this->m_children_.push_back(output);

  return output;
}

Hobbit::Variable *Hobbit::Variable::Flatten() {
  Shape flat_shape (1, 1, m_shape_.GetSize());

  Variable *output = new Variable (m_value_, m_type_, flat_shape, this);

  this->m_children_.push_back(output);

  return output;
}

void Hobbit::Variable::ClearChild(Hobbit::Variable *child) {

  auto iter = std::find(m_children_.begin(), m_children_.end(), child);
  if (iter != m_children_.end()) {
    m_children_.erase(iter);
  }

}

void Hobbit::Variable::ClearChildren() {
  this->m_children_.clear();
}

llvm::ArrayRef<llvm::Value *> Hobbit::Variable::Pack(llvm::IRBuilder<> &builder, uint32_t &vector_size) {
  llvm::Type *vector_type = llvm::VectorType::get(m_type_->getArrayElementType(), vector_size);
  uint32_t leftovers = m_shape_.GetSize() % vector_size;
  uint32_t num_vectors = (m_shape_.GetSize() - leftovers) / vector_size;

  std::vector<llvm::Value *> output;
  for (uint32_t i = 0; i < num_vectors; i++) {
    llvm::Value *tmp_vec = llvm::UndefValue::get(vector_type);
    for (uint32_t j = 0; j < vector_size; j++) {
      llvm::Value *elt = builder.CreateExtractValue(m_value_, i * vector_size + j);
      builder.CreateInsertElement(tmp_vec, elt, j);
    }
    output.push_back(tmp_vec);
  }

  return output;
}

//llvm::ArrayRef<Variable *>
//Hobbit::Variable::Split(llvm::IRBuilder<> &builder, uint32_t &chunk_size, uint32_t &stride, Axis &axis) {
//  uint32_t axis_size = m_shape_.GetAxisSize(axis);
//
//  Range k_range, h_range, w_range;
//  switch (axis) {
//    case K: break;
//    case H: break;
//    case W: break;
//  }
//}

//Hobbit::Variable Hobbit::Variable::GetCube(llvm::IRBuilder<> &builder, const uint32_t &n) {
//  uint32_t start_idx, end_idx;
//  m_shape_.GetCubeIdx(n, &start_idx, &end_idx);
//  const Shape cube_shape = m_shape_.GetCubeShape();
//
//  llvm::Type *subarray_type = llvm::ArrayType::get(m_type_->getArrayElementType(), cube_shape.GetSize());
//
//  llvm::Value *subarray = GetSubarray_(builder, subarray_type, start_idx, end_idx);
//
//  return Variable(subarray, subarray_type, cube_shape);
//
//}
//
//Hobbit::Variable Hobbit::Variable::GetPlane(llvm::IRBuilder<> &builder, const uint32_t &n, const uint32_t &k) {
//  uint32_t start_idx, end_idx;
//  m_shape_.GetPlaneIdx(n, k, &start_idx, &end_idx);
//  const Shape plane_shape = m_shape_.GetPlaneShape();
//
//  llvm::Type *subarray_type = llvm::ArrayType::get(m_type_->getArrayElementType(), plane_shape.GetSize());
//
//  llvm::Value *subarray = GetSubarray_(builder, subarray_type, start_idx, end_idx);
//
//  return Variable(subarray, subarray_type, plane_shape);
//}
//
//Hobbit::Variable
//Hobbit::Variable::GetVector(llvm::IRBuilder<> &builder, const uint32_t &n, const uint32_t &k, const uint32_t &h) {
//  uint32_t start_idx, end_idx;
//  m_shape_.GetPlaneIdx(n, k, &start_idx, &end_idx);
//  const Shape vector_shape = m_shape_.GetPlaneShape();
//
//  llvm::Type *subarray_type = llvm::ArrayType::get(m_type_->getArrayElementType(), vector_shape.GetSize());
//
//  llvm::Value *subarray = GetSubarray_(builder, subarray_type, start_idx, end_idx);
//
//  return Variable(subarray, subarray_type, vector_shape);
//}
//
//llvm::Value *Hobbit::Variable::GetSubarray_(llvm::IRBuilder<> &builder, llvm::Type *subarray_type, uint32_t &start_idx, uint32_t &end_idx) {
//
//  llvm::Value *subarray = builder.CreateAlloca(subarray_type);
//  for (uint32_t i = start_idx; i < end_idx; i++) {
//    llvm::Value *elt = builder.CreateLoad(builder.CreateGEP(m_value_, builder.getInt32(i)));
//    llvm::Value *subarray_elt = builder.CreateGEP(subarray, builder.getInt32(i - start_idx));
//    builder.CreateStore(elt, subarray_elt);
//  }
//
//  return subarray;
//}
//
//Hobbit::Variable Hobbit::Variable::Flatten() {
//  Shape flat = Shape(1, 1, 1, m_shape_.GetSize());
//  return Variable(m_value_, m_type_, flat);
//}
