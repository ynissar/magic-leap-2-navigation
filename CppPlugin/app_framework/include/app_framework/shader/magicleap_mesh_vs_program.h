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

static const char *kMagicLeapMeshVertexShader = R"GLSL(
  #version 410 core

  layout(std140) uniform Camera {
    mat4 view_proj;
    vec4 world_position;
  } camera;

  layout(std140) uniform Model {
    mat4 transform;
  } model;

  layout (location = 0) in vec3 position;
  layout (location = 1) in vec3 normal;
  layout (location = 4) in float confidence;

  layout (location = 0) out vec4 out_color;
  layout (location = 1) out vec4 out_normal;

  out gl_PerVertex {
    vec4 gl_Position;
  };

  void main() {
    gl_Position = camera.view_proj * model.transform * vec4(position, 1.0);
    out_color = mix(vec4(1, 0, 0, 1), vec4(0, 1, 0, 1), confidence);
    out_normal = camera.view_proj * model.transform * vec4(normal, 0.0);
  }
)GLSL";
}
}  // namespace ml
