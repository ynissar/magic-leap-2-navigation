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
#include <ml_types.h>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtx/quaternion.hpp>

namespace ml {
namespace app_framework {

class Pose {

public:
  glm::quat rotation;
  glm::vec3 position;

  // Basic ctors.
  Pose() : Pose(glm::quat_identity<float, glm::defaultp>(), glm::vec3()) {}
  Pose(const glm::quat& rot, const glm::vec3& pos) : rotation(rot), position(pos) {}

  // Ctors that copy the given field and use a default value for the other.
  Pose(const glm::quat& rot) : Pose(rot, glm::vec3()) {}
  Pose(const glm::vec3& pos) : Pose(glm::quat_identity<float, glm::defaultp>(), pos) {}

  // Ctor from MLTransform.
  Pose(const MLTransform& trans);

  // Copy/Move ctors and assignment operators.
  Pose(const Pose& other) : Pose(other.rotation, other.position) {}
  Pose(Pose&& other) : Pose(other.rotation, other.position) {}
  Pose& operator=(const Pose& other);
  Pose& operator=(Pose&& other);


  // Creates new Pose that is offset Pose in relation to the reference Pose
  friend Pose operator+(Pose reference, const Pose& offset);
  Pose Add(const Pose& reference, const Pose& offset);

  // Creates new Pose that strips all but the horizontal rotation.
  Pose HorizontalRotationOnly() const;
};

}  // namespace app_framework
}  // namespace ml
