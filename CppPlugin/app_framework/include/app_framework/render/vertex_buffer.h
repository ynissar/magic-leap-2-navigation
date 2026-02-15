// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Software License Agreement,
// located here: https://www.magicleap.com/software-license-agreement-ml2
// Terms and conditions applicable to third-party materials accompanying
// this distribution may also be found in the top-level NOTICE file
// appearing herein.
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once
#include "buffer.h"
#include "gl_type_size.h"
#include <app_framework/common.h>

#include <string>

namespace ml {
namespace app_framework {

class VertexBuffer final : public Buffer {
public:
  VertexBuffer(Buffer::Category category, GLint element_type, uint64_t element_cnt) :
      Buffer(category, GL_ARRAY_BUFFER),
      element_type_(element_type),
      element_cnt_(element_cnt) {
    element_size_ = GetGLTypeSize(element_type);
    vertex_size_ = element_size_ * element_cnt_;
  }
  ~VertexBuffer() = default;

  uint64_t GetElementCount() const {
    return element_cnt_;
  }

  GLint GetElementType() const {
    return element_type_;
  }

  uint64_t GetVertexSize() const {
    return vertex_size_;
  }

private:
  GLint element_type_;
  uint64_t element_cnt_;
  uint64_t element_size_;
  uint64_t vertex_size_;
};
}  // namespace app_framework
}  // namespace ml
