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
#include <app_framework/render/buffer.h>

namespace ml {
namespace app_framework {

Buffer::Buffer(Buffer::Category category, GLint gl_buffer_type) :
    category_(category),
    buffer_(0),
    gl_buffer_type_(gl_buffer_type) {
  gl_buffer_category_ = Buffer::GetGLBufferCategory(category);
  glGenBuffers(1, &buffer_);
}

Buffer::~Buffer() {
  if (buffer_) {
    glDeleteBuffers(1, &buffer_);
    buffer_ = 0;
  }
}

void Buffer::UpdateBuffer(const char *data, uint64_t size) {
  if (data != nullptr && size > 0) {
    size_ = size;
    glBindBuffer(gl_buffer_type_, buffer_);
    glBufferData(gl_buffer_type_, size, data, gl_buffer_category_);
  }
}
}  // namespace app_framework
}  // namespace ml
