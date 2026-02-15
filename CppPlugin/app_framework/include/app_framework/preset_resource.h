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
#include <vector>

#include "common.h"
#include "geometry/axis_mesh.h"
#include "geometry/cube_mesh.h"
#include "geometry/quad_mesh.h"

namespace ml {
namespace app_framework {

struct PresetResource {
  PresetResource() :
      meshes{
          std::make_shared<Axis>(),
          std::make_shared<CubeMesh>(),
          std::make_shared<QuadMesh>(),
      } {}
  std::vector<std::shared_ptr<Mesh>> meshes;
};

}  // namespace app_framework
}  // namespace ml
