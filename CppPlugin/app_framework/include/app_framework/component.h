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
#include "common.h"

namespace ml {
namespace app_framework {

class Node;

class Component : public RuntimeType {
  RUNTIME_TYPE_REGISTER(Component)
public:
  Component() = default;
  virtual ~Component() = default;

  void SetNode(std::shared_ptr<Node> node) {
    node_ = node;
  }

  std::shared_ptr<Node> GetNode() const {
    return node_;
  }

private:
  std::shared_ptr<Node> node_;
};
}  // namespace app_framework
}  // namespace ml
