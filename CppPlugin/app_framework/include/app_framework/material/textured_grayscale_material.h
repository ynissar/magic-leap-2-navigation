// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
// Copyright (c) 2022 Magic Leap, Inc. All Rights Reserved.
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
#include <app_framework/registry.h>
#include <app_framework/render/material.h>
#include <app_framework/render/texture.h>
#include <app_framework/shader/simple_textured_vs_program.h>

namespace ml {
namespace app_framework {

static const char *kSimpleTexturedGrayscaleFragmentShader = R"GLSL(
  #version 410 core

  uniform sampler2D Texture0;
  layout(std140) uniform Material {
    bool RemoveAlpha;
  } material;

  layout (location = 0) in vec2 in_tex_coords;

  layout (location = 0) out vec4 out_color;

  void main() {
    out_color = vec4(texture(Texture0, in_tex_coords).r);
    if (material.RemoveAlpha) {
      out_color.a = 0;
    } else {
      out_color.a = 1;
    }
  }
)GLSL";

class TexturedGrayscaleMaterial final : public Material {
public:
  TexturedGrayscaleMaterial(std::shared_ptr<Texture> tex) {
    SetVertexProgram(
        Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<VertexProgram>(kSimpleTexturedVertexShader));
    SetFragmentProgram(
        Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<FragmentProgram>(kSimpleTexturedGrayscaleFragmentShader));
    SetTexture0(tex);
  }
  ~TexturedGrayscaleMaterial() = default;

  MATERIAL_VARIABLE_DECLARE(bool, RemoveAlpha);
  MATERIAL_VARIABLE_DECLARE(std::shared_ptr<Texture>, Texture0);
};

}  // namespace app_framework
}  // namespace ml
