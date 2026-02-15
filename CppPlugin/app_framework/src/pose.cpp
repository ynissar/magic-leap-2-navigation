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

#include <app_framework/pose.h>
#include <app_framework/convert.h>

namespace ml {
namespace app_framework {

  Pose::Pose(const MLTransform& trans) : Pose(to_glm(trans.rotation), to_glm(trans.position)) {}

  Pose Pose::Add(const Pose& reference, const Pose& offset) {
    return reference + offset;
  }

  Pose operator+(Pose lhs, const Pose& rhs) {
    return {lhs.rotation * rhs.rotation, lhs.position + lhs.rotation * rhs.position};
  }

  Pose& Pose::operator=(const Pose& other) {
    rotation = other.rotation;
    position = other.position;
    return *this;
  }

  Pose& Pose::operator=(Pose&& other) {
    rotation = other.rotation;
    position = other.position;
    return *this;
  }

  Pose Pose::HorizontalRotationOnly() const {
    glm::quat rot = rotation;
    rot[0] = 0.0f;
    rot[2] = 0.0f;
    return Pose(glm::normalize(rot), position);
  }

}  // namespace app_framework
}  // namespace ml
