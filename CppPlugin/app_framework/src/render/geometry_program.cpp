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
#include <app_framework/render/geometry_program.h>
#include <app_framework/render/gl_type_size.h>

namespace ml {
namespace app_framework {

GeometryProgram::GeometryProgram(const char *code) : Program(code, GL_GEOMETRY_SHADER) {
  // Parse parameters
  glGetProgramiv(GetGLProgram(), GL_GEOMETRY_INPUT_TYPE, &type_);
}
}  // namespace app_framework
}  // namespace ml
