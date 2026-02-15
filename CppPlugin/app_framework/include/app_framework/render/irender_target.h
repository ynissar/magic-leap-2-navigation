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
#include <app_framework/common.h>

namespace ml {
namespace app_framework {

class IRenderTarget {
public:
  IRenderTarget(int32_t width, int32_t height, uint32_t color_layer_index, uint32_t depth_layer_index) :
      color_layer_index_(color_layer_index),
      depth_layer_index_(depth_layer_index),
      width_(width),
      height_(height) {}

  virtual ~IRenderTarget() {}

  virtual void BindTarget() const = 0;
  virtual void BindTargetForRead() const = 0;
  virtual void BindTargetForDraw() const = 0;

  virtual void AttachColorTextureLayer() const = 0;
  virtual void AttachDepthTextureLayer() const = 0;

  virtual void SetWidth(int32_t width) {
    width_ = width;
  }

  virtual void SetHeight(int32_t height) {
    height_ = height;
  }

  int32_t GetWidth() const {
    return width_;
  }

  int32_t GetHeight() const {
    return height_;
  }

  uint32_t GetColorTextureLayerIndex() const {
    return color_layer_index_;
  }

  uint32_t GetDepthTextureLayerIndex() const {
    return depth_layer_index_;
  }

protected:
  const uint32_t color_layer_index_;
  const uint32_t depth_layer_index_;
  int32_t width_;
  int32_t height_;
};

}  // namespace app_framework
}  // namespace ml
