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

namespace ml {
namespace app_framework {

static const char *kSolidColorFragmentShader = R"GLSL(
  #version 410 core

  layout(std140) uniform Material {
    vec4 Color;
    bool OverrideVertexColor;
  } material;

  layout (location = 0) in vec4 in_color;

  layout (location = 0) out vec4 out_color;

  void main() {
    if (material.OverrideVertexColor) {
      out_color = material.Color;
    } else {
      out_color = in_color;
    }
  }
)GLSL";
}
}  // namespace ml
