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

static const char *kSimpleTexturedFragmentShader = R"GLSL(
  #version 410 core

  uniform sampler2D Texture0;
  layout(std140) uniform Material {
    bool RemoveAlpha;
  } material;

  layout (location = 0) in vec2 in_tex_coords;

  layout (location = 0) out vec4 out_color;

  void main() {
    out_color = texture(Texture0, in_tex_coords);
    if (material.RemoveAlpha) {
      out_color.a = 0;
    }
  }
)GLSL";

}
}  // namespace ml
