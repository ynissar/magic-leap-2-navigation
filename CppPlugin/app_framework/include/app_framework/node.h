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
#include "component.h"
#include "pose.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ml {
namespace app_framework {

enum Space { Local, World };

class Node : public std::enable_shared_from_this<Node> {
public:
  Node();
  Node(const glm::vec3 &);
  ~Node(){};
  Node(Node &&other);
  Node(const Node &) = delete;
  Node &operator=(const Node &other) = delete;
  Node &operator=(Node &&other);

  bool AddChild(std::shared_ptr<Node> new_child);
  bool RemoveChild(std::shared_ptr<Node> child);
  const std::vector<std::shared_ptr<Node>> &GetChildren() const;

  void SetLocalPose(const Pose &pose);
  void SetWorldPose(const Pose &pose);
  void SetLocalTranslation(const glm::vec3 &translation);
  void SetWorldTranslation(const glm::vec3 &translation);
  void SetLocalRotation(const glm::quat &rotation);
  void SetWorldRotation(const glm::quat &rotation);
  void SetLocalScale(const glm::vec3 &scale);

  Pose GetLocalPose() const;
  Pose GetWorldPose() const;
  const glm::mat4 &GetLocalTransform() const;
  const glm::mat4 &GetWorldTransform() const;
  const glm::vec3 &GetLocalTranslation() const;
  const glm::vec3 GetWorldTranslation() const;
  const glm::quat &GetLocalRotation() const;
  const glm::quat GetWorldRotation() const;
  const glm::vec3 &GetLocalScale() const;

  void UpdateWorldPosition();
  std::string GetName();
  void SetName(std::string);
  bool IsDirty() const;
  void SetDirty();

  void AddComponent(std::shared_ptr<Component> component);

  const std::vector<std::shared_ptr<Component>> &GetComponents() const {
    return components_;
  }

  template <typename ComponentType>
  std::shared_ptr<ComponentType> GetComponent() const {
    auto it = components_by_type_.find(ComponentType::GetClassRuntimeType());
    if (components_by_type_.end() == it) {
      return nullptr;
    }
    return std::static_pointer_cast<ComponentType>(it->second);
  }

  std::weak_ptr<Node> GetParent() {
    return parent_;
  }

private:
  glm::vec3 local_translation_;
  glm::quat local_rotation_;
  glm::vec3 local_scale_;
  mutable glm::mat4 local_transform_;
  mutable glm::mat4 world_transform_;

  const glm::mat4 GetParentWorldTransform() const;

  std::vector<std::shared_ptr<Node>> child_list_;
  std::string name_;
  std::weak_ptr<Node> parent_;
  mutable bool world_dirty_;
  mutable bool local_dirty_;

  std::vector<std::shared_ptr<Component>> components_;
  std::unordered_map<uint64_t, std::shared_ptr<Component>> components_by_type_;
};

}  // namespace app_framework
}  // namespace ml
