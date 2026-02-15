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
#include "program.h"
#include <app_framework/common.h>

namespace ml {
namespace app_framework {

// GL fragment program
class FragmentProgram final : public Program {
public:
  // Initialize the program with null-terminated code string
  FragmentProgram(const char *code) : Program(code, GL_FRAGMENT_SHADER) {}
  virtual ~FragmentProgram() = default;
};
}  // namespace app_framework
}  // namespace ml
