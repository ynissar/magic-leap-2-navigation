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
#include <app_framework/render/render_target.h>

namespace ml {
namespace app_framework {

RenderTarget::~RenderTarget() {
  if (gl_framebuffer_) {
    glDeleteFramebuffers(1, &gl_framebuffer_);
  }
}

void RenderTarget::InitializeFramebuffer() {
  gl_framebuffer_ = 0;
  if (color_) {
    width_ = color_->GetWidth();
    height_ = color_->GetHeight();
    glGenFramebuffers(1, &gl_framebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, gl_framebuffer_);

    AttachColorTextureLayer();
    AttachDepthTextureLayer();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
}

void RenderTarget::BindTarget() const {
  glBindFramebuffer(GL_FRAMEBUFFER, gl_framebuffer_);
}

void RenderTarget::BindTargetForRead() const {
  glBindFramebuffer(GL_READ_FRAMEBUFFER, gl_framebuffer_);
}

void RenderTarget::BindTargetForDraw() const {
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl_framebuffer_);
}

void RenderTarget::AttachColorTextureLayer() const {
  if (color_) {
    auto color_texture_type = color_->GetTextureType();
    if (color_texture_type == GL_TEXTURE_2D_ARRAY) {
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_->GetGLTexture(), 0, color_layer_index_);
    } else if (color_texture_type == GL_TEXTURE_2D) {
      glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_->GetGLTexture(), 0);
    }
  } else {
    ALOGW("AttachColorTextureLayer: no color texture initialized");
  }
}

void RenderTarget::AttachDepthTextureLayer() const {
  // Only do depth when we have color
  if (depth_) {
    auto depth_texture_type = depth_->GetTextureType();
    if (depth_texture_type == GL_TEXTURE_2D_ARRAY) {
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_->GetGLTexture(), 0, depth_layer_index_);
    } else if (depth_texture_type == GL_TEXTURE_2D) {
      glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_->GetGLTexture(), 0);
    }
  } else {
    ALOGW("AttachDepthTextureLayer: no depth texture initialized");
  }
}

}  // namespace app_framework
}  // namespace ml
