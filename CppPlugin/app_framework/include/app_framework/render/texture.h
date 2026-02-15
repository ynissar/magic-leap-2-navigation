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

class Texture final {
public:
  Texture(GLint texture_type, GLuint tex, int32_t width, int32_t height, bool owned = false) :
      texture_(tex),
      texture_type_(texture_type),
      width_(width),
      height_(height),
      owned_(owned) {}

  Texture() : Texture(GL_TEXTURE_2D, 0, 0, 0, false) {}
  ~Texture();

  void SetWidth(int32_t width) {
    width_ = width;
  }

  void SetHeight(int32_t height) {
    height_ = height;
  }

  int32_t GetWidth() const {
    return width_;
  }

  int32_t GetHeight() const {
    return height_;
  }

  GLenum GetTextureType() const {
    return texture_type_;
  }

  GLuint GetGLTexture() const {
    return texture_;
  }

private:
  GLuint texture_;
  GLint texture_type_;
  int32_t width_;
  int32_t height_;
  bool owned_;
};
}  // namespace app_framework
}  // namespace ml
