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
#include "asset_manager.h"
#include "common.h"
#include "resource_pool.h"

namespace ml {
namespace app_framework {

class Registry final {
public:
  Registry() = default;
  ~Registry() = default;

  static const std::unique_ptr<Registry> &GetInstance();

  void Initialize(IAssetManagerPtr assetManager);
  void Cleanup();

  const std::unique_ptr<ResourcePool> &GetResourcePool() const {
    return pool_;
  }

private:
  std::unique_ptr<ResourcePool> pool_;
};
}  // namespace app_framework
}  // namespace ml
