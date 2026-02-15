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
#include <app_framework/shader/solid_color_fs_program.h>
#include <app_framework/shader/solid_color_vs_program.h>

namespace ml {
namespace app_framework {

class FlatMaterial final : public Material {
public:
  FlatMaterial(const glm::vec4 &color) {
    SetVertexProgram(
        Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<VertexProgram>(kSolidColorVertexShader));
    SetFragmentProgram(
        Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<FragmentProgram>(kSolidColorFragmentShader));
    SetColor(color);
  }
  ~FlatMaterial() = default;

  MATERIAL_VARIABLE_DECLARE(bool, OverrideVertexColor);
  MATERIAL_VARIABLE_DECLARE(glm::vec4, Color);
};
}  // namespace app_framework
}  // namespace ml
