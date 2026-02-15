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
#include <app_framework/render/material.h>
#include <app_framework/render/mesh.h>

namespace ml {
namespace app_framework {

// Renderable component
class RenderableComponent final : public Component {
  RUNTIME_TYPE_REGISTER(RenderableComponent)
public:
  RenderableComponent(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material) :
      visible_(true),
      mesh_(mesh),
      material_(material) {}
  ~RenderableComponent() = default;

  void SetVisible(bool visible) {
    visible_ = visible;
  }

  void SetMesh(std::shared_ptr<Mesh> mesh) {
    mesh_ = mesh;
  }

  void SetMaterial(std::shared_ptr<Material> material) {
    material_ = material;
  }

  bool GetVisible() const {
    return visible_;
  }

  std::shared_ptr<Mesh> GetMesh() const {
    return mesh_;
  }

  std::shared_ptr<Material> GetMaterial() const {
    return material_;
  }

private:
  bool visible_;
  std::shared_ptr<Mesh> mesh_;
  std::shared_ptr<Material> material_;
};

}  // namespace app_framework
}  // namespace ml
