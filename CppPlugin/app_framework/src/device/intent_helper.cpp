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
#include <app_framework/system_helper_common.h>

namespace ml {
namespace app_framework {

IntentHelper::IntentHelper(struct android_app *app) :
    vm_(app->activity->vm),
    context_(0),
    native_activity_(0),
    intent_cls_(0),
    intent_obj_(0),
    add_flags_mid_(0),
    set_action_mid_(0),
    set_package_mid_(0),
    add_category_mid_(0),
    set_type_mid_(0),
    put_extra_strarray_mid_(0),
    put_extra_int_mid_(0),
    component_cls_(0),
    new_component_mid_(0),
    set_component_mid_(0),
    send_broadcast_mid_(0),
    start_activity_mid_(0),
    uri_cls_(0),
    uri_parse_mid_(0),
    set_data_mid_(0) {
  ScopedJNIAttach jni_scope(vm_);
  native_activity_ = app->activity->clazz;

  //  cache classes
  jclass cls = GET_OBJECT_CLASS(true, native_activity_);
  context_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(cls));
  jni_scope->DeleteLocalRef(cls);

  // Get an instance of Intent
  cls = FIND_CLASS(true, "android/content/Intent");
  intent_cls_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(cls));
  jni_scope->DeleteLocalRef(cls);

  jmethodID mid = GET_METHOD_ID(true, intent_cls_, "<init>", "()V");
  jobject obj = NEW_OBJECT(true, intent_cls_, mid);

  intent_obj_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(obj));

  add_flags_mid_ = GET_METHOD_ID(true, intent_cls_, "addFlags", "(I)Landroid/content/Intent;");
  add_category_mid_ = GET_METHOD_ID(true, intent_cls_, "addCategory", "(Ljava/lang/String;)Landroid/content/Intent;");
  set_type_mid_ = GET_METHOD_ID(true, intent_cls_, "setType", "(Ljava/lang/String;)Landroid/content/Intent;");
  set_data_mid_ = GET_METHOD_ID(true, intent_cls_, "setData", "(Landroid/net/Uri;)Landroid/content/Intent;");
  set_action_mid_ = GET_METHOD_ID(true, intent_cls_, "setAction", "(Ljava/lang/String;)Landroid/content/Intent;");
  set_package_mid_ = GET_METHOD_ID(true, intent_cls_, "setPackage", "(Ljava/lang/String;)Landroid/content/Intent;");
  put_extra_strarray_mid_ = GET_METHOD_ID(true, intent_cls_, "putExtra",
                                          "(Ljava/lang/String;[Ljava/lang/String;)Landroid/content/Intent;");
  put_extra_int_mid_ = GET_METHOD_ID(true, intent_cls_, "putExtra", "(Ljava/lang/String;I)Landroid/content/Intent;");

  // Get an instance of the ComponentName class
  cls = FIND_CLASS(true, "android/content/ComponentName");
  component_cls_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(cls));
  new_component_mid_ = GET_METHOD_ID(true, cls, "<init>", "(Ljava/lang/String;Ljava/lang/String;)V");
  set_component_mid_ = GET_METHOD_ID(true, intent_cls_, "setComponent",
                                     "(Landroid/content/ComponentName;)Landroid/content/Intent;");
  send_broadcast_mid_ = GET_METHOD_ID(true, context_, "sendBroadcast", "(Landroid/content/Intent;)V");
  start_activity_mid_ = GET_METHOD_ID(true, context_, "startActivity", "(Landroid/content/Intent;)V");
  jni_scope->DeleteLocalRef(cls);

  // Get an instance of Uri
  cls = FIND_CLASS(true, "android/net/Uri");
  uri_cls_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(cls));
  jni_scope->DeleteLocalRef(cls);

  uri_parse_mid_ = GET_STATIC_METHOD_ID(true, uri_cls_, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
}

IntentHelper::~IntentHelper() {
  ScopedJNIAttach jni_scope(vm_);

  if (intent_cls_) jni_scope->DeleteGlobalRef(intent_cls_);
  if (intent_obj_) jni_scope->DeleteGlobalRef(intent_obj_);
  if (context_) jni_scope->DeleteGlobalRef(context_);
  if (component_cls_) jni_scope->DeleteGlobalRef(component_cls_);
  if (uri_cls_) jni_scope->DeleteGlobalRef(uri_cls_);
}

IntentHelper &IntentHelper::AddFlags(int flag) {
  ScopedJNIAttach jni_scope(vm_);
  CALL_OBJECT_METHOD(true, intent_obj_, add_flags_mid_, flag);
  return *this;
}

IntentHelper &IntentHelper::SetData(const std::string &uri_str) {
  ScopedJNIAttach jni_scope(vm_);

  jstring str = jni_scope->NewStringUTF(uri_str.c_str());
  jobject uri = CALL_STATIC_OBJECT_METHOD(false, uri_cls_, uri_parse_mid_, str);
  jni_scope->DeleteLocalRef(str);

  CALL_OBJECT_METHOD(true, intent_obj_, set_data_mid_, uri);
  return *this;
}

IntentHelper &IntentHelper::SetAction(const std::string &action_str) {
  ScopedJNIAttach jni_scope(vm_);
  jstring action = jni_scope->NewStringUTF(action_str.c_str());

  CALL_OBJECT_METHOD(true, intent_obj_, set_action_mid_, action);

  jni_scope->DeleteLocalRef(action);
  return *this;
}

IntentHelper &IntentHelper::AddCategory(const std::string &category_str) {
  ScopedJNIAttach jni_scope(vm_);
  jstring category = jni_scope->NewStringUTF(category_str.c_str());

  CALL_OBJECT_METHOD(true, intent_obj_, add_category_mid_, category);

  jni_scope->DeleteLocalRef(category);
  return *this;
}

IntentHelper &IntentHelper::SetType(const std::string &type_str) {
  ScopedJNIAttach jni_scope(vm_);
  jstring type = jni_scope->NewStringUTF(type_str.c_str());

  CALL_OBJECT_METHOD(true, intent_obj_, set_type_mid_, type);

  jni_scope->DeleteLocalRef(type);
  return *this;
}

IntentHelper &IntentHelper::SetPackage(const std::string &package_str) {
  ScopedJNIAttach jni_scope(vm_);
  jstring package = jni_scope->NewStringUTF(package_str.c_str());

  CALL_OBJECT_METHOD(true, intent_obj_, set_package_mid_, package);

  jni_scope->DeleteLocalRef(package);
  return *this;
}

IntentHelper &IntentHelper::PutExtra(const std::string &key_str, const std::vector<std::string> &value_str_array) {
  ScopedJNIAttach jni_scope(vm_);
  jstring key = jni_scope->NewStringUTF(key_str.c_str());
  jobjectArray value = (jobjectArray)jni_scope->NewObjectArray(value_str_array.size(),
                                                               jni_scope->FindClass("java/lang/String"),
                                                               jni_scope->NewStringUTF(""));
  for (uint32_t i = 0; i < value_str_array.size(); i++) {
    jstring str = jni_scope->NewStringUTF(value_str_array[i].c_str());
    jni_scope->SetObjectArrayElement(value, i, str);
    jni_scope->DeleteLocalRef(str);
  }
  CALL_OBJECT_METHOD(true, intent_obj_, put_extra_strarray_mid_, key, value);

  jni_scope->DeleteLocalRef(key);
  jni_scope->DeleteLocalRef(value);
  return *this;
}

IntentHelper &IntentHelper::PutExtra(const std::string &key_str, int value) {
  ScopedJNIAttach jni_scope(vm_);
  jstring key = jni_scope->NewStringUTF(key_str.c_str());
  CALL_OBJECT_METHOD(true, intent_obj_, put_extra_int_mid_, key, value);

  jni_scope->DeleteLocalRef(key);
  return *this;
}

IntentHelper &IntentHelper::SetComponent(const std::string &pkg_name_str, const std::string &cls_name_str) {
  ScopedJNIAttach jni_scope(vm_);
  jstring pkgName = jni_scope->NewStringUTF(pkg_name_str.c_str());
  jstring clsName = jni_scope->NewStringUTF(cls_name_str.c_str());
  jobject component_name_obj = NEW_OBJECT(true, component_cls_, new_component_mid_, pkgName, clsName);
  CALL_VOID_METHOD(true, component_name_obj, new_component_mid_, pkgName, clsName);

  // Calling intent.setComponentName passing in pkg+class name
  CALL_OBJECT_METHOD(true, intent_obj_, set_component_mid_, component_name_obj);

  jni_scope->DeleteLocalRef(pkgName);
  jni_scope->DeleteLocalRef(clsName);
  jni_scope->DeleteLocalRef(component_name_obj);
  return *this;
}

void IntentHelper::SendBroadcast() {
  ScopedJNIAttach jni_scope(vm_);
  CALL_VOID_METHOD(true, native_activity_, send_broadcast_mid_, intent_obj_);
}

void IntentHelper::StartActivity() {
  ScopedJNIAttach jni_scope(vm_);
  CALL_VOID_METHOD(true, native_activity_, start_activity_mid_, intent_obj_);
}

void IntentHelper::StartActivity(const std::string &activity_str) {
  ScopedJNIAttach jni_scope(vm_);
  jclass cls = FIND_CLASS(true, activity_str.c_str());
  jmethodID mid = GET_METHOD_ID(true, cls, "startActivity", "(Landroid/content/Intent;)V");
  CALL_VOID_METHOD(true, native_activity_, mid, intent_obj_);

  jni_scope->DeleteLocalRef(cls);
}

}  // namespace app_framework
}  // namespace ml
