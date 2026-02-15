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

static const char *kSimpleTexturedVertexShader = R"GLSL(
  #version 410 core

  layout(std140) uniform Camera {
    mat4 view_proj;
    vec4 world_position;
  } camera;

  layout(std140) uniform Model {
    mat4 transform;
  } model;

  layout (location = 0) in vec3 position;
  layout (location = 2) in vec2 tex_coords;

  out gl_PerVertex {
      vec4 gl_Position;
  };

  layout (location = 0) out vec2 out_tex_coords;

  void main() {
    gl_Position = camera.view_proj * model.transform * vec4(position, 1.0);
    out_tex_coords = tex_coords;
  }
)GLSL";

}
}  // namespace ml
