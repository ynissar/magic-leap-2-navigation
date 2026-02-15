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

static const char* kSimpleConfidenceFragmentShader = R"GLSL(
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

float Normalize(in float value, in float minpoint, in float endpoint) {
  return InverseLerp(value, minpoint, endpoint);
}

float NormalizeConfidence(in float confidence) {
  float conf = clamp(abs(confidence), 0.0, 0.5);
  return Normalize(conf, 0.0, 0.5);
}

float NormalizeDepth(in float depth_meters) {
  return Normalize(depth_meters, material.MinDepth, material.MaxDepth);
}

vec3 GetConfidenceColor(in float conf) {
  return vec3(conf, 1.0 - conf, 0.0);
}

void main() {
  float depth_meters = texture2D(DepthTexture, in_tex_coords.xy).r;
  float confidence = texture2D(HelperTexture, in_tex_coords.xy).r;
  float normalized_confidence = NormalizeConfidence(confidence);
  float normalized_depth = NormalizeDepth(depth_meters);

  vec4 depth_color = vec4(GetConfidenceColor(normalized_confidence), 1.0);
  depth_color.rgb *= (1.0 - normalized_depth / 2.0);
  out_color = depth_color;
}
)GLSL";

class ConfidenceMaterial final : public Material {
public:
    ConfidenceMaterial(std::shared_ptr<Texture> depth_texture, std::shared_ptr<Texture> confidence_texture, float min_depth = 0.f, float max_depth = 5.f) {
      SetVertexProgram(
          Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<VertexProgram>(kSimpleTexturedVertexShader));
      SetFragmentProgram(
          Registry::GetInstance()->GetResourcePool()->LoadShaderFromCode<FragmentProgram>(kSimpleConfidenceFragmentShader));
      SetDepthTexture(depth_texture);
      SetHelperTexture(confidence_texture);
      SetMinDepth(min_depth);
      SetMaxDepth(max_depth);
  }
  ~ConfidenceMaterial() = default;

  MATERIAL_VARIABLE_DECLARE(float, MinDepth);
  MATERIAL_VARIABLE_DECLARE(float, MaxDepth);
  MATERIAL_VARIABLE_DECLARE(std::shared_ptr<Texture>, DepthTexture);
  MATERIAL_VARIABLE_DECLARE(std::shared_ptr<Texture>, HelperTexture);
};

}  // namespace app_framework
}  // namespace ml
