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
#include <app_framework/common.h>

namespace ml {
namespace app_framework {

class IndexBuffer final : public Buffer {
public:
  IndexBuffer(Buffer::Category category, GLint type) :
      Buffer(category, GL_ELEMENT_ARRAY_BUFFER),
      type_(type),
      index_cnt_(0) {
    switch (type) {
      case GL_UNSIGNED_BYTE: index_size_ = 1; break;
      case GL_UNSIGNED_SHORT: index_size_ = 2; break;
      case GL_UNSIGNED_INT: index_size_ = 4; break;
      default: ALOGE("unhandled type (%x)", type);
    }
  }
  ~IndexBuffer() = default;

  void UpdateBuffer(const char *data, uint64_t size) override {
    Buffer::UpdateBuffer(data, size);
    index_cnt_ = size / index_size_;
  }

  GLint GetIndexType() const {
    return type_;
  }

  uint64_t GetIndexSize() const {
    return index_size_;
  }

  uint64_t GetIndexCount() const {
    return index_cnt_;
  }

private:
  GLint type_;
  uint64_t index_size_;
  uint64_t index_cnt_;
};
}  // namespace app_framework
}  // namespace ml
