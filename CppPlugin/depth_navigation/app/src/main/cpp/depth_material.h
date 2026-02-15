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

static const char* kSimpleDepthFragmentShader = R"GLSL(
#version 410 core

precision mediump float;

layout(std140) uniform Material {
  float MinDepth;
  float MaxDepth;
} material;

uniform sampler2D DepthTexture;
uniform sampler2D HelperTexture;

layout (location = 0) in vec2 in_tex_coords;

layout (location = 0) out vec4 out_color;

float InverseLerp(float value, float min, float max) {
  return clamp((value - min) / (max - min), 0.0, 1.0);
}

// The input x should be normalized in range 0 to 1.
vec3 GetColorVisualization(in float x) {
  return texture2D(HelperTexture, vec2(x, 0.5)).rgb;
}

float NormalizeDepth(in float depth_meters) {
  return InverseLerp(depth_meters, material.MinDepth, material.MaxDepth);
}

void main() {
  float depth_meters = texture2D(DepthTexture, in_tex_coords.xy).r;
  float normalized_depth = NormalizeDepth(depth_meters);
  vec4 depth_color = vec4(GetColorVisualization(normalized_depth), 1.0);

  // Values outside of range mapped to black.
  if (depth_meters < material.MinDepth || depth_meters > material.MaxDepth) {
    depth_color.rgb *= 0.0;
  }
  out_color = depth_color;
}
)GLSL";

class DepthMaterial final : public Material {
public:
  DepthMaterial(std::shared_ptr<Texture> depth_texture, std::shared_ptr<Texture> color_map, float min_depth = 0.f, float max_depth = 5.f) {
    SetVertexProgram(
        Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<VertexProgram>(kSimpleTexturedVertexShader));
    SetFragmentProgram(
        Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<FragmentProgram>(kSimpleDepthFragmentShader));
    SetDepthTexture(depth_texture);
    SetHelperTexture(color_map);
    SetMinDepth(min_depth);
    SetMaxDepth(max_depth);
  }
  ~DepthMaterial() = default;

  MATERIAL_VARIABLE_DECLARE(float, MinDepth);
  MATERIAL_VARIABLE_DECLARE(float, MaxDepth);
  MATERIAL_VARIABLE_DECLARE(std::shared_ptr<Texture>, DepthTexture);
  MATERIAL_VARIABLE_DECLARE(std::shared_ptr<Texture>, HelperTexture);
};

}  // namespace app_framework
}  // namespace ml
