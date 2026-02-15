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
#pragma once

#include <android_native_app_glue.h>
#include <app_framework/logging.h>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef ML_LUMIN
#include <jni.h>
#endif

namespace ml {
namespace app_framework {

class IntentHelper {
private:
#if ML_LUMIN
  JavaVM *vm_;
  jclass context_;
  jobject native_activity_;
  jclass intent_cls_;
  jobject intent_obj_;
  jmethodID add_flags_mid_;
  jmethodID set_action_mid_;
  jmethodID set_package_mid_;
  jmethodID add_category_mid_;
  jmethodID set_type_mid_;
  jmethodID put_extra_strarray_mid_;
  jmethodID put_extra_int_mid_;
  jclass component_cls_;
  jmethodID new_component_mid_;
  jmethodID set_component_mid_;
  jmethodID send_broadcast_mid_;
  jmethodID start_activity_mid_;
  jclass uri_cls_;
  jmethodID uri_parse_mid_;
  jmethodID set_data_mid_;
#endif
public:
  IntentHelper(struct android_app *app);
  ~IntentHelper();
  IntentHelper &SetAction(const std::string &action_str);
  IntentHelper &SetPackage(const std::string &package_str);
  IntentHelper &AddFlags(int flag);
  IntentHelper &AddCategory(const std::string &category_str);
  IntentHelper &SetData(const std::string &uri_str);
  IntentHelper &SetType(const std::string &type_str);
  IntentHelper &PutExtra(const std::string &key_str, const std::vector<std::string> &value_str_array);
  IntentHelper &PutExtra(const std::string &key_str, int value);
  IntentHelper &SetComponent(const std::string &pkg_name_str, const std::string &cls_name_str);

  void SendBroadcast();
  void StartActivity();
  void StartActivity(const std::string &activity_str);
};

}  // namespace app_framework
}  // namespace ml
