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

#include <app_framework/logging.h>
#include <cassert>
#include <jni.h>
#include <string>

namespace ml {
namespace app_framework {

#define CHECK_FOR_ERRORS(is_fatal, name)                                      \
  if (jni_scope->ExceptionCheck()) {                                          \
    jni_scope->ExceptionDescribe();                                           \
    jni_scope->ExceptionClear();                                              \
    is_fatal ? ALOGF("%s() exception, see logcat for more details.", #name) : \
               ALOGI("%s() exception, see logcat for more details.", #name);  \
  }

#define FIND_CLASS(is_fatal, ...)    \
  jni_scope->FindClass(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, FindClass)

#define GET_METHOD_ID(is_fatal, ...)   \
  jni_scope->GetMethodID(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, GetMethodID)

#define GET_STATIC_METHOD_ID(is_fatal, ...)  \
  jni_scope->GetStaticMethodID(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, GetStaticMethodID)

#define CALL_OBJECT_METHOD(is_fatal, ...)   \
  jni_scope->CallObjectMethod(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, CallObjectMethod)

#define CALL_STATIC_LONG_METHOD(is_fatal, ...)  \
  jni_scope->CallStaticLongMethod(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, CallStaticLongMethod)

#define CALL_STATIC_VOID_METHOD(is_fatal, ...)  \
  jni_scope->CallStaticVoidMethod(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, CallStaticVoidMethod)

#define CALL_STATIC_OBJECT_METHOD(is_fatal, ...)  \
  jni_scope->CallStaticObjectMethod(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, CallStaticObjectMethod)

#define GET_OBJECT_CLASS(is_fatal, ...)   \
  jni_scope->GetObjectClass(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, GetObjectClass)

#define GET_FIELD_ID(is_fatal, ...)   \
  jni_scope->GetFieldID(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, GetFieldID)

#define GET_STATIC_INT_FIELD(is_fatal, ...)  \
  jni_scope->GetStaticIntField(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, GetStaticIntField)

#define GET_OBJECT_FIELD(is_fatal, ...)   \
  jni_scope->GetObjectField(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, GetObjectField)

#define GET_STATIC_OBJECT_FIELD(is_fatal, ...)  \
  jni_scope->GetStaticObjectField(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, GetStaticObjectField)

#define CALL_NONVIRTUAL_OBJECT_METHOD(is_fatal, ...)  \
  jni_scope->CallNonvirtualObjectMethod(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, CallNonvirtualObjectMethod)

#define GET_INT_FIELD(is_fatal, ...)   \
  jni_scope->GetIntField(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, GetIntField)

#define CALL_INT_METHOD(is_fatal, ...)   \
  jni_scope->CallIntMethod(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, CallIntMethod)

#define CALL_LONG_METHOD(is_fatal, ...)   \
  jni_scope->CallLongMethod(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, CallLongMethod)

#define CALL_BOOLEAN_METHOD(is_fatal, ...)   \
  jni_scope->CallBooleanMethod(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, CallBooleanMethod)

#define CALL_VOID_METHOD(is_fatal, ...)   \
  jni_scope->CallVoidMethod(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, CallVoidMethod)

#define NEW_OBJECT(is_fatal, ...)    \
  jni_scope->NewObject(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, NewObject)

#define GET_STATIC_FIELD_ID(is_fatal, ...)  \
  jni_scope->GetStaticFieldID(__VA_ARGS__); \
  CHECK_FOR_ERRORS(is_fatal, GetStaticFieldID)

/*!
    \brief Scoped JNI Attach helper.
    Keeps the current thread attached to the JNI for the life of the
    object.

    \code{.cpp}
    void something( struct android_app *state ) {
        // outside of attached scope.
        ScopedJNIAttach jni_scope(state->activity->vm);
        // This thread is attached to the JNI env and can be used!
        url_cls_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(jni_scope->FindClass("java/net/URL")));
        // when jni_scope goes out of scope, the thread is detached automatically if it need to be
    }
    \endcode
*/
class ScopedJNIAttach {
public:
  /*! \brief Operator-> returns JNI Environment Variable. The Java Environment is used to interact with JVM.
   */
  JNIEnv *operator->() {
    assert(jni_env_ != nullptr);
    return jni_env_;
  }

  /*! \brief ScopedJNIAttach constrctor.
      Attaches the current thread to the JVM if it hasn't already been attached.
      \param vm, a pointer to the Java Virtual Machine.
  */
  ScopedJNIAttach(JavaVM *vm) : jni_env_(nullptr), vm_(vm), should_detach_(false) {
    assert(vm_ != nullptr);
    if (vm_->GetEnv((void **)&jni_env_, JNI_VERSION_1_6) == JNI_EDETACHED) {
      if (vm_->AttachCurrentThread(&jni_env_, NULL) != JNI_OK) {
        ALOGE("ScopedJNIAttach: Unable to attach current thread");
        return;
      }
      should_detach_ = true;
    }
  }
  /*! \brief ScopedJNIAttach destructor.
      Detaches the thread from the JVM IF the constructor
      attached. If the thread was attached prior to the
      constructor being called, it will be left attached.
  */
  ~ScopedJNIAttach() {
    assert(vm_ != nullptr);
    if (should_detach_) {
      vm_->DetachCurrentThread();
    }
  }

private:
  JNIEnv *jni_env_;
  JavaVM *vm_;
  bool should_detach_;
};

/*!
  \brief Utility Function to convert from jsring to std::string.

  \param env, ScopedJNIAttach reference
  \param jstring which will be converted to std::string

  \return std::string converted from jsting
*/
std::string ToString(ScopedJNIAttach &env, const jstring jstr);

}  // namespace app_framework
}  // namespace ml
