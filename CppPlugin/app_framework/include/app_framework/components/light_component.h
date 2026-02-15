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
#include <app_framework/component.h>
#include <app_framework/render/texture.h>

namespace ml {
namespace app_framework {

enum class LightType { Point = 0, Directional };

// Light component
class LightComponent final : public Component {
  RUNTIME_TYPE_REGISTER(LightComponent)
public:
  constexpr static const int MAXIMUM_LIGHTS = 32;

  LightComponent() :
      color_(1.0, 1.0, 1.0),
      direction_(glm::normalize(glm::vec3(0, -1, -1))),
      light_type_(LightType::Point),
      light_strength_(1.0f){};
  ~LightComponent() = default;

  void SetDirection(const glm::vec3 &direction) {
    direction_ = direction;
  }

  // Only valid for direcional light
  glm::vec3 GetDirection() const {
    return direction_;
  }

  void SetLightColor(const glm::vec3 &color) {
    color_ = color;
  }

  glm::vec3 GetLightColor() const {
    return color_;
  }

  void SetLightType(LightType type) {
    light_type_ = type;
  }

  void SetLightStrength(float strength) {
    light_strength_ = strength;
  }

  float GetLightStrength() const {
    return light_strength_;
  }

  LightType GetLightType() const {
    return light_type_;
  }

private:
  glm::vec3 color_;
  glm::vec3 direction_;
  LightType light_type_;
  float light_strength_;
};

}  // namespace app_framework
}  // namespace ml
