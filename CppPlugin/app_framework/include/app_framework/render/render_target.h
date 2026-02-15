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
#include "irender_target.h"
#include "texture.h"
#include <app_framework/common.h>

namespace ml {
namespace app_framework {

class RenderTarget final : public IRenderTarget {
public:
  RenderTarget(int32_t width, int32_t height) : IRenderTarget(width, height, 0, 0), gl_framebuffer_(0) {}

  RenderTarget(std::shared_ptr<Texture> color, std::shared_ptr<Texture> depth, uint32_t color_layer_index,
               uint32_t depth_layer_index) :
      IRenderTarget(0, 0, color_layer_index, depth_layer_index),
      color_(color),
      depth_(depth),
      gl_framebuffer_(0) {
    InitializeFramebuffer();
  }
  ~RenderTarget();

  std::shared_ptr<Texture> GetColorTexture() const {
    return color_;
  }

  std::shared_ptr<Texture> GetDepthTexture() const {
    return depth_;
  }

  void BindTarget() const override;
  void BindTargetForRead() const override;
  void BindTargetForDraw() const override;

  void AttachColorTextureLayer() const override;
  void AttachDepthTextureLayer() const override;

  void SetWidth(int32_t width) override {
    if (color_) {
      ALOGW("Setting render target size to a non-default framebuffer");
    }
    width_ = width;
  }

  void SetHeight(int32_t height) override {
    if (color_) {
      ALOGW("Setting render target size to a non-default framebuffer");
    }
    height_ = height;
  }

private:
  void InitializeFramebuffer();
  std::shared_ptr<Texture> color_;
  std::shared_ptr<Texture> depth_;
  GLuint gl_framebuffer_;
};

}  // namespace app_framework
}  // namespace ml
