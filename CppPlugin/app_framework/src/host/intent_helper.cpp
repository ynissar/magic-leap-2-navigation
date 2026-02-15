// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
// Copyright (c) 2021 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Software License Agreement,
// located here: https://www.magicleap.com/software-license-agreement-ml2
// Terms and conditions applicable to third-party materials accompanying
// this distribution may also be found in the top-level NOTICE file
// appearing herein.
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%

#include <app_framework/intent_helper.h>

namespace ml {
namespace app_framework {

IntentHelper::IntentHelper(struct android_app *app) {
  // App Sim code goes here
  (void)app;
}

IntentHelper::~IntentHelper() {}

IntentHelper &IntentHelper::AddFlags(int flag) {
  // App Sim code goes here
  (void)flag;
  return *this;
}

IntentHelper &IntentHelper::SetData(const std::string &uri_str) {
  // App Sim code goes here
  (void)uri_str;
  return *this;
}

IntentHelper &IntentHelper::SetAction(const std::string &action_str) {
  // App Sim code goes here
  (void)action_str;
  return *this;
}

IntentHelper &IntentHelper::AddCategory(const std::string &category_str) {
  // App Sim code goes here
  (void)category_str;
  return *this;
}

IntentHelper &IntentHelper::SetType(const std::string &type_str) {
  // App Sim code goes here
  (void)type_str;
  return *this;
}

IntentHelper &IntentHelper::SetPackage(const std::string &package_str) {
  // App Sim code goes here
  (void)package_str;
  return *this;
}

IntentHelper &IntentHelper::PutExtra(const std::string &key_str, const std::vector<std::string> &value_str_array) {
  // App Sim code goes here
  (void)key_str;
  (void)value_str_array;
  return *this;
}

IntentHelper &IntentHelper::PutExtra(const std::string &key_str, int value) {
  // App Sim code goes here
  (void)key_str;
  (void)value;
  return *this;
}

IntentHelper &IntentHelper::SetComponent(const std::string &pkg_name_str, const std::string &cls_name_str) {
  // App Sim code goes here
  (void)pkg_name_str;
  (void)cls_name_str;
  return *this;
}

void IntentHelper::SendBroadcast() {
  // App Sim code goes here
}

void IntentHelper::StartActivity() {
  // App Sim code goes here
}

void IntentHelper::StartActivity(const std::string &activity_str) {
  // App Sim code goes here
  (void)activity_str;
}

}  // namespace app_framework
}  // namespace ml
