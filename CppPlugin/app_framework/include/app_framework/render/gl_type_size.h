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
#include <cinttypes>

#include <glad/glad.h>

#include <app_framework/ml_macros.h>

namespace ml {
namespace app_framework {

static inline uint64_t GetGLTypeSize(GLint type) {
  uint64_t size = 0;
  switch (type) {
    case GL_BYTE:
    case GL_BOOL:
    case GL_UNSIGNED_BYTE: size = 1; break;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT: size = 2; break;
    case GL_INT:
    case GL_FLOAT:
    case GL_UNSIGNED_INT: size = 4; break;
    case GL_FLOAT_VEC2: size = 8; break;
    case GL_FLOAT_VEC3: size = 12; break;
    case GL_FLOAT_VEC4: size = 16; break;
    case GL_FLOAT_MAT4: size = 64; break;
    case GL_SAMPLER_2D: size = 0; break;
#ifdef ML_LUMIN
    case GL_SAMPLER_EXTERNAL_OES: size = 0; break;
#endif
    default: ALOGE("unhandled type (%x)", type);
  }
  return size;
}
}  // namespace app_framework
}  // namespace ml
