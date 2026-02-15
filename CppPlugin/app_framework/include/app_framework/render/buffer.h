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
#include <app_framework/common.h>
#include <glad/glad.h>

namespace ml {
namespace app_framework {

class Buffer {
public:
  enum class Category {
    Static,
    Dynamic,
  };

  Buffer(Buffer::Category category, GLint gl_buffer_type);
  virtual ~Buffer();

  Category GetCategory() const {
    return category_;
  }
  uint64_t GetSize() const {
    return size_;
  }

  GLuint GetGLBuffer() const {
    return buffer_;
  }

  virtual void UpdateBuffer(const char *data, uint64_t size);

  GLint GetGLBufferType() const {
    return gl_buffer_type_;
  }

  inline GLint GetGLBufferCategory() const {
    return GetGLBufferCategory(GetCategory());
  }

  static inline GLint GetGLBufferCategory(Category category) {
    switch (category) {
      case Category::Static: return GL_STATIC_DRAW;
      case Category::Dynamic: return GL_DYNAMIC_DRAW;
    }
    return 0;
  }

private:
  Category category_;
  GLuint buffer_;
  GLint gl_buffer_type_;
  GLint gl_buffer_category_;
  uint64_t size_;
};
}  // namespace app_framework
}  // namespace ml
