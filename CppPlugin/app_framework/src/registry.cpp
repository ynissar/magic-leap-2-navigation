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
#include <app_framework/registry.h>

namespace ml {
namespace app_framework {

void Registry::Initialize(IAssetManagerPtr asset_manager) {
  pool_.reset(new ResourcePool());
  pool_->InitializePresetResources(asset_manager);
}

void Registry::Cleanup() {
  if (pool_) {
    pool_.reset();
  }
}

const std::unique_ptr<Registry> &Registry::GetInstance() {
  static std::unique_ptr<Registry> instance(new Registry());
  return instance;
}

}  // namespace app_framework
}  // namespace ml
