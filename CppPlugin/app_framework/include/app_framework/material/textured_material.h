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
#include <app_framework/registry.h>
#include <app_framework/render/material.h>
#include <app_framework/render/texture.h>
#include <app_framework/shader/simple_textured_fs_program.h>
#include <app_framework/shader/simple_textured_vs_program.h>

namespace ml {
namespace app_framework {

class TexturedMaterial final : public Material {
public:
  TexturedMaterial(std::shared_ptr<Texture> tex) {
    SetVertexProgram(
        Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<VertexProgram>(kSimpleTexturedVertexShader));
    SetFragmentProgram(
        Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<FragmentProgram>(kSimpleTexturedFragmentShader));
    SetTexture0(tex);
  }
  ~TexturedMaterial() = default;

  MATERIAL_VARIABLE_DECLARE(bool, RemoveAlpha);
  MATERIAL_VARIABLE_DECLARE(std::shared_ptr<Texture>, Texture0);
};

}  // namespace app_framework
}  // namespace ml
