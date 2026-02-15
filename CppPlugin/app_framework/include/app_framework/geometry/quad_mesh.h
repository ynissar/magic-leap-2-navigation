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
#include <array>

#include <app_framework/common.h>
#include <app_framework/render/mesh.h>

namespace ml {
namespace app_framework {

class QuadMesh final : public Mesh {
  RUNTIME_TYPE_REGISTER(QuadMesh)
  const std::array<glm::vec3, 4> g_vertex_positions{{
      {-0.5f, -0.5f, 0.0f},
      {-0.5f, +0.5f, 0.0f},
      {+0.5f, -0.5f, 0.0f},
      {+0.5f, +0.5f, 0.0f},
  }};

  const std::array<std::array<uint16_t, 3>, 2> g_indices{{
      {{0, 1, 2}},
      {{3, 2, 1}},
  }};

  const std::array<glm::vec2, 4> g_tex_coords = {{
      {0.0f, 0.0f},
      {0.0f, 1.0f},
      {1.0f, 0.0f},
      {1.0f, 1.0f},
  }};

public:
  QuadMesh() : Mesh(Buffer::Category::Static, GL_UNSIGNED_SHORT) {
    UpdateMesh(g_vertex_positions.data(), nullptr, g_vertex_positions.size(), (uint16_t *)g_indices.data(),
               g_indices[0].size() * g_indices.size());
    UpdateTexCoordsBuffer(g_tex_coords.data());
  }
  ~QuadMesh() = default;
};

}  // namespace app_framework
}  // namespace ml
