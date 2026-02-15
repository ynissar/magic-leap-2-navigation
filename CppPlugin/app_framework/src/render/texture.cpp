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
#include "stb_image.h"
#include <app_framework/render/texture.h>

namespace ml {
namespace app_framework {

Texture::~Texture() {
  if (owned_) {
    glDeleteTextures(1, &texture_);
    texture_ = 0;
  }
}

}  // namespace app_framework
}  // namespace ml
