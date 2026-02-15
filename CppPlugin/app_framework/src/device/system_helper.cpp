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

#include <app_framework/logging.h>
#include <app_framework/system_helper.h>
#include <app_framework/system_helper_common.h>

namespace ml {
namespace app_framework {

/*
  Macros for querying battery properties
  taken from : https://developer.android.com/reference/kotlin/android/os/BatteryManager#constants
*/
#define BATTERY_PROPERTY_CAPACITY 4
#define BATTERY_PROPERTY_STATUS   6

SystemHelper::SystemHelper(struct android_app *app) :
    version_code_(-1),
    vm_(app->activity->vm),
    activity_mng_cls_(nullptr),
    context_(nullptr),
    network_cls_(nullptr),
    pkg_mng_cls_(nullptr),
    get_batt_property_mid_(nullptr),
    get_batt_charge_time_mid_(nullptr),
    get_avail_disk_bytes_mid_(nullptr),
    get_avail_external_bytes_mid_(nullptr),
    get_memory_state_mid_(nullptr),
    get_total_disk_bytes_mid_(nullptr),
    get_total_external_bytes_mid_(nullptr),
    registerReceiver_mid_(nullptr),
    batt_mng_obj_(nullptr),
    batt_changed_int_obj_(nullptr),
    controller_intent_filter_(nullptr),
    powerbank_intent_filter_(nullptr),
    native_activity_(nullptr),
    statsf_external_storage_obj(nullptr),
    statsf_internal_storage_obj(nullptr),
    pwr_mgr_(nullptr),
    is_interactive_mid_(nullptr),
    pkg_info_obj_(nullptr),
    app_info_obj_(nullptr) {
  ScopedJNIAttach jni_scope(vm_);

  native_activity_ = app->activity->clazz;

  //  cache classes
  jclass cls = GET_OBJECT_CLASS(true, native_activity_);
  context_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(cls));
  jni_scope->DeleteLocalRef(cls);

  cls = FIND_CLASS(true, "android/content/pm/PackageManager");
  pkg_mng_cls_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(cls));
  jni_scope->DeleteLocalRef(cls);

  cls = FIND_CLASS(true, "android/net/ConnectivityManager");
  network_cls_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(cls));
  jni_scope->DeleteLocalRef(cls);

  cls = FIND_CLASS(true, "android/os/BatteryManager");
  jmethodID mid = GET_METHOD_ID(true, cls, "<init>", "()V");
  get_batt_property_mid_ = GET_METHOD_ID(true, cls, "getIntProperty", "(I)I");
  get_batt_charge_time_mid_ = GET_METHOD_ID(true, cls, "computeChargeTimeRemaining", "()J");
  jobject obj = jni_scope->NewObject(cls, mid);
  batt_mng_obj_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(obj));
  jni_scope->DeleteLocalRef(cls);
  jni_scope->DeleteLocalRef(obj);

  jstring controller_batt_intent_name = jni_scope->NewStringUTF("com.magicleap.controller.action.BATTERY_CHANGED");
  cls = FIND_CLASS(true, "android/content/IntentFilter");
  jmethodID cls_constructor = GET_METHOD_ID(true, cls, "<init>", "(Ljava/lang/String;)V");
  obj = jni_scope->NewObject(cls, cls_constructor, controller_batt_intent_name);
  controller_intent_filter_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(obj));
  registerReceiver_mid_ = GET_METHOD_ID(true, context_, "registerReceiver",
                                        "(Landroid/content/BroadcastReceiver;Landroid/content/IntentFilter;)Landroid/"
                                        "content/Intent;");

  jstring powerbank_intent_name = jni_scope->NewStringUTF("com.magicleap.powerbank.action.BATTERY_CHANGED");
  cls = FIND_CLASS(true, "android/content/IntentFilter");
  cls_constructor = GET_METHOD_ID(true, cls, "<init>", "(Ljava/lang/String;)V");
  obj = jni_scope->NewObject(cls, cls_constructor, powerbank_intent_name);
  powerbank_intent_filter_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(obj));

  jstring action_batt_changed_int = jni_scope->NewStringUTF("android.intent.action.BATTERY_CHANGED");
  cls = FIND_CLASS(true, "android/content/IntentFilter");
  cls_constructor = GET_METHOD_ID(true, cls, "<init>", "(Ljava/lang/String;)V");
  obj = jni_scope->NewObject(cls, cls_constructor, action_batt_changed_int);
  batt_changed_int_obj_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(obj));

  jni_scope->DeleteLocalRef(controller_batt_intent_name);
  jni_scope->DeleteLocalRef(powerbank_intent_name);

  jclass android_os_env_class = FIND_CLASS(true, "android/os/Environment");
  jmethodID get_root_dir_mid = GET_STATIC_METHOD_ID(true, android_os_env_class, "getRootDirectory", "()Ljava/io/File;");
  jobject file_dir = CALL_STATIC_OBJECT_METHOD(true, android_os_env_class, get_root_dir_mid);
  jclass android_file_class = GET_OBJECT_CLASS(true, file_dir);
  jmethodID get_abs_dir_mid = GET_METHOD_ID(true, android_file_class, "getAbsolutePath", "()Ljava/lang/String;");
  jstring file_path_str = (jstring)CALL_OBJECT_METHOD(true, file_dir, get_abs_dir_mid);
  jclass android_os_statfs_class = FIND_CLASS(true, "android/os/StatFs");
  cls_constructor = GET_METHOD_ID(true, android_os_statfs_class, "<init>", "(Ljava/lang/String;)V");
  obj = jni_scope->NewObject(android_os_statfs_class, cls_constructor, file_path_str);
  statsf_internal_storage_obj = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(obj));
  get_total_disk_bytes_mid_ = GET_METHOD_ID(true, android_os_statfs_class, "getTotalBytes", "()J");

  get_avail_disk_bytes_mid_ = GET_METHOD_ID(true, android_os_statfs_class, "getAvailableBytes", "()J");

  get_root_dir_mid = GET_STATIC_METHOD_ID(true, android_os_env_class, "getExternalStorageDirectory",
                                          "()Ljava/io/File;");
  file_dir = CALL_STATIC_OBJECT_METHOD(true, android_os_env_class, get_root_dir_mid);
  android_file_class = GET_OBJECT_CLASS(true, file_dir);
  get_abs_dir_mid = GET_METHOD_ID(true, android_file_class, "getAbsolutePath", "()Ljava/lang/String;");
  file_path_str = (jstring)CALL_OBJECT_METHOD(true, file_dir, get_abs_dir_mid)
      cls_constructor = GET_METHOD_ID(true, android_os_statfs_class, "<init>", "(Ljava/lang/String;)V");
  obj = jni_scope->NewObject(android_os_statfs_class, cls_constructor, file_path_str);
  statsf_external_storage_obj = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(obj));
  get_total_external_bytes_mid_ = GET_METHOD_ID(true, android_os_statfs_class, "getTotalBytes", "()J");

  jni_scope->DeleteLocalRef(action_batt_changed_int);

  get_avail_external_bytes_mid_ = GET_METHOD_ID(true, android_os_statfs_class, "getAvailableBytes", "()J");
  jni_scope->DeleteLocalRef(file_path_str);
  jni_scope->DeleteLocalRef(android_os_statfs_class);
  jni_scope->DeleteLocalRef(android_file_class);
  jni_scope->DeleteLocalRef(file_dir);
  jni_scope->DeleteLocalRef(android_os_env_class);

  cls = FIND_CLASS(true, "android/content/Context");
  mid = GET_METHOD_ID(true, cls, "getApplicationContext", "()Landroid/content/Context;");
  obj = CALL_OBJECT_METHOD(true, native_activity_, mid);
  mid = GET_METHOD_ID(true, cls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
  jobject activity_manager = CALL_OBJECT_METHOD(true, obj, mid, jni_scope->NewStringUTF("activity"));
  activity_manager_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(activity_manager));
  cls = FIND_CLASS(true, "android/app/ActivityManager");
  activity_mng_cls_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(cls));
  get_memory_state_mid_ = GET_STATIC_METHOD_ID(true, activity_mng_cls_, "getMyMemoryState",
                                               "(Landroid/app/ActivityManager$RunningAppProcessInfo;)V");
  jclass app_info_class = FIND_CLASS(true, "android/app/ActivityManager$RunningAppProcessInfo");
  cls_constructor = GET_METHOD_ID(true, app_info_class, "<init>", "()V");
  jobject running_process_info_obj = jni_scope->NewObject(app_info_class, cls_constructor);
  running_process_info_obj_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(running_process_info_obj));
  jclass running_process_info_class = GET_OBJECT_CLASS(true, running_process_info_obj);
  last_trim_level_fid_ = GET_FIELD_ID(true, running_process_info_class, "lastTrimLevel", "I");

  jni_scope->DeleteLocalRef(running_process_info_class);
  jni_scope->DeleteLocalRef(running_process_info_obj);
  jni_scope->DeleteLocalRef(app_info_class);
  jni_scope->DeleteLocalRef(activity_manager);
  jni_scope->DeleteLocalRef(cls);
  jni_scope->DeleteLocalRef(obj);
}

SystemHelper::~SystemHelper() {
  ScopedJNIAttach jni_scope(vm_);
  jni_scope->DeleteGlobalRef(activity_mng_cls_);
  jni_scope->DeleteGlobalRef(pkg_info_obj_);
  jni_scope->DeleteGlobalRef(app_info_obj_);
  jni_scope->DeleteGlobalRef(context_);
  jni_scope->DeleteGlobalRef(pkg_mng_cls_);
  jni_scope->DeleteGlobalRef(network_cls_);
  jni_scope->DeleteGlobalRef(pwr_mgr_);
  jni_scope->DeleteGlobalRef(batt_mng_obj_);
  jni_scope->DeleteGlobalRef(controller_intent_filter_);
  jni_scope->DeleteGlobalRef(powerbank_intent_filter_);
  jni_scope->DeleteGlobalRef(activity_manager_);
  jni_scope->DeleteGlobalRef(batt_changed_int_obj_);
  jni_scope->DeleteGlobalRef(running_process_info_obj_);
  jni_scope->DeleteGlobalRef(statsf_external_storage_obj);
  jni_scope->DeleteGlobalRef(statsf_internal_storage_obj);
}

const std::string &SystemHelper::GetPackageName() const {
  if (package_name_.empty()) {
    ScopedJNIAttach jni_scope(vm_);

    jmethodID mid = GET_METHOD_ID(true, context_, "getPackageName", "()Ljava/lang/String;");

    jstring str = (jstring)CALL_OBJECT_METHOD(true, native_activity_, mid);

    package_name_ = ToString(jni_scope, str);

    jni_scope->DeleteLocalRef(str);
  }

  return package_name_;
}

int SystemHelper::GetVersionCode() const {
  if (version_code_ == -1) {
    ScopedJNIAttach jni_scope(vm_);

    jclass cls = FIND_CLASS(true, "android/content/pm/PackageInfo");

    jfieldID fid = GET_FIELD_ID(true, cls, "versionCode", "I");

    version_code_ = GET_INT_FIELD(true, GetPackageInfo(), fid);

    jni_scope->DeleteLocalRef(cls);
  }

  return version_code_;
}

const std::string &SystemHelper::GetVersionName() const {
  if (version_name_.empty()) {
    ScopedJNIAttach jni_scope(vm_);

    jclass cls = FIND_CLASS(true, "android/content/pm/PackageInfo");

    jfieldID fid = GET_FIELD_ID(true, cls, "versionName", "Ljava/lang/String;");

    jstring str = (jstring)GET_OBJECT_FIELD(true, GetPackageInfo(), fid);

    version_name_ = ToString(jni_scope, str);

    jni_scope->DeleteLocalRef(str);
    jni_scope->DeleteLocalRef(cls);
  }

  return version_name_;
}

const std::string &SystemHelper::GetApplicationInstallDir() const {
  if (app_install_dir_.empty()) {
    ScopedJNIAttach jni_scope(vm_);

    jclass cls = FIND_CLASS(true, "android/content/pm/ApplicationInfo");

    jfieldID fid = GET_FIELD_ID(true, cls, "sourceDir", "Ljava/lang/String;");

    jstring str = (jstring)GET_OBJECT_FIELD(true, GetAppInfo(), fid);

    app_install_dir_ = ToString(jni_scope, str);

    jni_scope->DeleteLocalRef(str);
    jni_scope->DeleteLocalRef(cls);
  }

  return app_install_dir_;
}

const std::string &SystemHelper::GetApplicationWritableDir() const {
  if (app_writable_dir_.empty()) {
    ScopedJNIAttach jni_scope(vm_);

    jclass cls = FIND_CLASS(true, "android/content/pm/ApplicationInfo");

    jfieldID fid = GET_FIELD_ID(true, cls, "dataDir", "Ljava/lang/String;");

    jstring str = (jstring)GET_OBJECT_FIELD(true, GetAppInfo(), fid);

    app_writable_dir_ = ToString(jni_scope, str);

    jni_scope->DeleteLocalRef(str);
    jni_scope->DeleteLocalRef(cls);
  }

  return app_writable_dir_;
}

const std::string &SystemHelper::GetExternalFilesDir() const {
  if (external_files_dir_.empty()) {
    ScopedJNIAttach jni_scope(vm_);

    // Implements: File f = this.getExternalFilesDir(null);
    jmethodID method = GET_METHOD_ID(true, context_, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");

    jobject file_object = CALL_OBJECT_METHOD(true, native_activity_, method, nullptr);

    // Implements: String path = f.getAbsolutePath();
    jclass file_class = FIND_CLASS(true, "java/io/File");

    method = GET_METHOD_ID(true, file_class, "getAbsolutePath", "()Ljava/lang/String;");

    jstring string_object = (jstring)CALL_OBJECT_METHOD(true, file_object, method);

    external_files_dir_ = ToString(jni_scope, string_object);

    jni_scope->DeleteLocalRef(file_object);
    jni_scope->DeleteLocalRef(file_class);
    jni_scope->DeleteLocalRef(string_object);
  }

  return external_files_dir_;
}

const std::string &SystemHelper::GetISO3Language() const {
  if (language_.empty()) {
    ScopedJNIAttach jni_scope(vm_);

    jclass locale_cls = FIND_CLASS(true, "java/util/Locale");
    jmethodID get_default_mid = GET_STATIC_METHOD_ID(true, locale_cls, "getDefault", "()Ljava/util/Locale;");
    jobject default_locale_obj = CALL_STATIC_OBJECT_METHOD(true, locale_cls, get_default_mid);

    jmethodID get_iso3_language_mid = GET_METHOD_ID(true, locale_cls, "getISO3Language", "()Ljava/lang/String;");
    jstring language_jstr = (jstring)CALL_OBJECT_METHOD(true, default_locale_obj, get_iso3_language_mid);

    language_ = ToString(jni_scope, language_jstr);

    jni_scope->DeleteLocalRef(locale_cls);
    jni_scope->DeleteLocalRef(default_locale_obj);
    jni_scope->DeleteLocalRef(language_jstr);
  }

  return language_;
}

bool SystemHelper::IsInteractive() const {
  ScopedJNIAttach jni_scope(vm_);
  if (!pwr_mgr_) {
    jclass cls = FIND_CLASS(true, "android/content/Context");

    jmethodID mid = GET_METHOD_ID(true, cls, "getApplicationContext", "()Landroid/content/Context;");

    jobject obj = CALL_OBJECT_METHOD(true, native_activity_, mid);

    mid = GET_METHOD_ID(true, cls, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jni_scope->DeleteLocalRef(cls);

    jstring str = jni_scope->NewStringUTF("power");
    jobject obj1 = CALL_OBJECT_METHOD(true, obj, mid, str);
    jni_scope->DeleteLocalRef(obj);
    jni_scope->DeleteLocalRef(str);

    pwr_mgr_ = jni_scope->NewGlobalRef(obj1);
    jni_scope->DeleteLocalRef(obj1);

    cls = FIND_CLASS(true, "android/os/PowerManager");
    is_interactive_mid_ = GET_METHOD_ID(true, cls, "isInteractive", "()Z");
    jni_scope->DeleteLocalRef(cls);
  }

  jboolean is_interactive = CALL_BOOLEAN_METHOD(true, pwr_mgr_, is_interactive_mid_);

  return (is_interactive == JNI_TRUE);
}

bool SystemHelper::CheckApplicationPermission(const std::string &permission_name) const {
  ScopedJNIAttach jni_scope(vm_);
  jint permission_granted = jint(-1);

  jfieldID fid = GET_STATIC_FIELD_ID(true, pkg_mng_cls_, "PERMISSION_GRANTED", "I");

  permission_granted = GET_STATIC_INT_FIELD(true, pkg_mng_cls_, fid);

  jclass cls = FIND_CLASS(true, "android/content/Context");

  jmethodID mid = GET_METHOD_ID(true, cls, "checkSelfPermission", "(Ljava/lang/String;)I");

  jint res = CALL_INT_METHOD(true, native_activity_, mid, jni_scope->NewStringUTF(permission_name.c_str()));

  jni_scope->DeleteLocalRef(cls);

  return res == permission_granted;
}

void SystemHelper::RequestApplicationPermission(const std::string &permission_name) const {
  std::vector<std::string> permissions{permission_name};
  RequestApplicationPermissions(permissions);
}

void SystemHelper::RequestApplicationPermissions(const std::vector<std::string> &permissions) const {
  ScopedJNIAttach jni_scope(vm_);

  jobjectArray permissions_array = jni_scope->NewObjectArray(permissions.size(),
                                                             jni_scope->FindClass("java/lang/String"),
                                                             jni_scope->NewStringUTF(""));

  for (size_t i = 0; i < permissions.size(); i++) {
    jni_scope->SetObjectArrayElement(permissions_array, i, jni_scope->NewStringUTF(permissions[i].c_str()));
  }

  jclass cls = FIND_CLASS(true, "android/app/Activity");

  jmethodID mid = GET_METHOD_ID(true, cls, "requestPermissions", "([Ljava/lang/String;I)V");

  CALL_VOID_METHOD(true, native_activity_, mid, permissions_array, 0);

  jni_scope->DeleteLocalRef(permissions_array);
  jni_scope->DeleteLocalRef(cls);
}

std::unordered_map<std::string, std::string> &SystemHelper::GetIntentExtras() const {
  ScopedJNIAttach jni_scope(vm_);
  intent_extras_.clear();

  static jmethodID get_intent_mid = GET_METHOD_ID(true, context_, "getIntent", "()Landroid/content/Intent;");

  jobject intent = CALL_OBJECT_METHOD(true, native_activity_, get_intent_mid);  // intent object

  jclass cls = GET_OBJECT_CLASS(true, intent);

  static jmethodID get_extras_mid = GET_METHOD_ID(true, cls, "getExtras", "()Landroid/os/Bundle;");

  jobject extras = (jstring)CALL_OBJECT_METHOD(true, intent, get_extras_mid);  //  intent extras
  if (extras) {
    jclass bundle_cls = GET_OBJECT_CLASS(true, extras);  //  class pointer of Bundle

    static jmethodID key_set_mid = GET_METHOD_ID(true, bundle_cls, "keySet", "()Ljava/util/Set;");

    jobject key_set = (jobject)CALL_NONVIRTUAL_OBJECT_METHOD(true, extras, bundle_cls, key_set_mid);
    if (key_set) {
      jclass iks = GET_OBJECT_CLASS(true, key_set);

      static jmethodID to_arrary_mid = GET_METHOD_ID(true, iks, "toArray", "()[Ljava/lang/Object;");

      jobjectArray array = (jobjectArray)CALL_NONVIRTUAL_OBJECT_METHOD(true, key_set, iks, to_arrary_mid);

      int size = jni_scope->GetArrayLength(array);
      for (int i = 0; i < size; ++i) {
        jstring keyj = (jstring)jni_scope->GetObjectArrayElement(array, i);

        std::string key = ToString(jni_scope, keyj);
        intent_extras_[key] = {};

        static jmethodID get_string_mid = GET_METHOD_ID(true, bundle_cls, "getString",
                                                        "(Ljava/lang/String;)Ljava/lang/String;");

        jstring value = (jstring)CALL_NONVIRTUAL_OBJECT_METHOD(true, extras, bundle_cls, get_string_mid, keyj);
        if (value) {
          intent_extras_[key] = ToString(jni_scope, value);
        }
      }
    }
  }

  return intent_extras_;
}

jobject SystemHelper::GetPackageInfo() const {
  if (!pkg_info_obj_) {
    ScopedJNIAttach jni_scope(vm_);

    jmethodID mid = GET_METHOD_ID(true, context_, "getPackageManager", "()Landroid/content/pm/PackageManager;");

    jobject obj = CALL_OBJECT_METHOD(true, native_activity_, mid);

    mid = GET_METHOD_ID(true, pkg_mng_cls_, "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");

    jobject pi_obj = CALL_OBJECT_METHOD(true, obj, mid, jni_scope->NewStringUTF(GetPackageName().c_str()), 0x0);

    pkg_info_obj_ = jni_scope->NewGlobalRef(pi_obj);

    jni_scope->DeleteLocalRef(obj);
    jni_scope->DeleteLocalRef(pi_obj);
  }

  return pkg_info_obj_;
}

jobject SystemHelper::GetAppInfo() const {
  if (!app_info_obj_) {
    ScopedJNIAttach jni_scope(vm_);

    jmethodID mid = GET_METHOD_ID(true, context_, "getPackageManager", "()Landroid/content/pm/PackageManager;");

    jobject obj = CALL_OBJECT_METHOD(true, native_activity_, mid);

    mid = GET_METHOD_ID(true, pkg_mng_cls_, "getApplicationInfo",
                        "(Ljava/lang/String;I)Landroid/content/pm/ApplicationInfo;");

    jobject ai_obj = CALL_OBJECT_METHOD(true, obj, mid, jni_scope->NewStringUTF(GetPackageName().c_str()), 0x0);

    app_info_obj_ = jni_scope->NewGlobalRef(ai_obj);

    jni_scope->DeleteLocalRef(obj);
    jni_scope->DeleteLocalRef(ai_obj);
  }

  return app_info_obj_;
}

int SystemHelper::GetComputePackBatteryLevel() const {
  ScopedJNIAttach jni_scope(vm_);
  return CALL_INT_METHOD(true, batt_mng_obj_, get_batt_property_mid_, BATTERY_PROPERTY_CAPACITY);
}

int SystemHelper::GetComputePackBatteryStatus() const {
  ScopedJNIAttach jni_scope(vm_);
  return CALL_INT_METHOD(true, batt_mng_obj_, get_batt_property_mid_, BATTERY_PROPERTY_STATUS);
}

float SystemHelper::GetComputePackBatteryTemperature() const {
  ScopedJNIAttach jni_scope(vm_);
  jobject intent_object = CALL_OBJECT_METHOD(true, native_activity_, registerReceiver_mid_, nullptr,
                                             batt_changed_int_obj_);
  jclass intent_class = GET_OBJECT_CLASS(true, intent_object);
  jmethodID compute_pack_temp_mid = GET_METHOD_ID(true, intent_class, "getIntExtra", "(Ljava/lang/String;I)I");
  float temp = (float)CALL_INT_METHOD(true, intent_object, compute_pack_temp_mid,
                                      jni_scope->NewStringUTF("temperature"), 0);
  // Intent output is an integer which is accurate to a tenth of a centigrade, for example, 39.6 Degrees Celcius as
  // "396".
  return temp / 10.0f;
}

long SystemHelper::GetComputePackChargeTimeRemaining() const {
  ScopedJNIAttach jni_scope(vm_);
  return CALL_LONG_METHOD(true, batt_mng_obj_, get_batt_charge_time_mid_);
}

bool SystemHelper::IsControllerPresent() const {
  ScopedJNIAttach jni_scope(vm_);
  jobject intent_object = CALL_OBJECT_METHOD(false, native_activity_, registerReceiver_mid_, nullptr,
                                             controller_intent_filter_);
  if (intent_object == nullptr) {
    return false;
  }

  jclass intent_class = GET_OBJECT_CLASS(false, intent_object);
  jmethodID get_controller_batt_presence_mid = GET_METHOD_ID(true, intent_class, "getBooleanExtra",
                                                             "(Ljava/lang/String;Z)Z");
  return CALL_BOOLEAN_METHOD(true, intent_object, get_controller_batt_presence_mid, jni_scope->NewStringUTF("present"),
                             false);
}

int SystemHelper::GetControllerBatteryLevel() const {
  ScopedJNIAttach jni_scope(vm_);
  jobject intent_object = CALL_OBJECT_METHOD(false, native_activity_, registerReceiver_mid_, nullptr,
                                             controller_intent_filter_);
  if (intent_object == nullptr) {
    return 0;
  }

  jclass intent_class = GET_OBJECT_CLASS(false, intent_object);
  jmethodID get_controller_batt_int_extra_mid = GET_METHOD_ID(true, intent_class, "getIntExtra",
                                                              "(Ljava/lang/String;I)I");
  return CALL_INT_METHOD(true, intent_object, get_controller_batt_int_extra_mid, jni_scope->NewStringUTF("level"), 0);
}

int SystemHelper::GetControllerBatteryStatus() const {
  ScopedJNIAttach jni_scope(vm_);
  jobject intent_object = CALL_OBJECT_METHOD(false, native_activity_, registerReceiver_mid_, nullptr,
                                             controller_intent_filter_);
  if (intent_object == nullptr) {
    return false;
  }

  jclass intent_class = GET_OBJECT_CLASS(false, intent_object);
  jmethodID get_controller_batt_int_extra_mid = GET_METHOD_ID(true, intent_class, "getIntExtra",
                                                              "(Ljava/lang/String;I)I");
  return CALL_INT_METHOD(true, intent_object, get_controller_batt_int_extra_mid, jni_scope->NewStringUTF("status"), 0);
}

bool SystemHelper::IsPowerBankPresent() const {
  ScopedJNIAttach jni_scope(vm_);
  jobject intent_object = CALL_OBJECT_METHOD(false, native_activity_, registerReceiver_mid_, nullptr,
                                             powerbank_intent_filter_);
  if (intent_object == nullptr) {
    return false;
  }

  jclass intent_class = GET_OBJECT_CLASS(false, intent_object);
  jmethodID get_powerbank_batt_presence_mid = GET_METHOD_ID(true, intent_class, "getBooleanExtra",
                                                            "(Ljava/lang/String;Z)Z");
  return CALL_BOOLEAN_METHOD(true, intent_object, get_powerbank_batt_presence_mid, jni_scope->NewStringUTF("present"),
                             false);
}

int SystemHelper::GetPowerBankBatteryLevel() const {
  ScopedJNIAttach jni_scope(vm_);
  jobject intent_object = CALL_OBJECT_METHOD(false, native_activity_, registerReceiver_mid_, nullptr,
                                             powerbank_intent_filter_);
  if (intent_object == nullptr) {
    return 0;
  }

  jclass intent_class = GET_OBJECT_CLASS(false, intent_object);
  jmethodID get_powerbank_batt_int_extra_mid = GET_METHOD_ID(true, intent_class, "getIntExtra",
                                                             "(Ljava/lang/String;I)I");
  return CALL_INT_METHOD(true, intent_object, get_powerbank_batt_int_extra_mid, jni_scope->NewStringUTF("level"), 0);
}

long SystemHelper::GetTotalDiskBytes() const {
  ScopedJNIAttach jni_scope(vm_);
  return CALL_LONG_METHOD(true, statsf_internal_storage_obj, get_total_disk_bytes_mid_);
}

long SystemHelper::GetTotalExternalBytes() const {
  ScopedJNIAttach jni_scope(vm_);
  return CALL_LONG_METHOD(true, statsf_external_storage_obj, get_total_external_bytes_mid_);
}

long SystemHelper::GetAvailableDiskBytes() const {
  ScopedJNIAttach jni_scope(vm_);
  return CALL_LONG_METHOD(true, statsf_internal_storage_obj, get_avail_disk_bytes_mid_);
}

long SystemHelper::GetAvailableExternalBytes() const {
  ScopedJNIAttach jni_scope(vm_);
  return CALL_LONG_METHOD(true, statsf_external_storage_obj, get_avail_external_bytes_mid_);
}

int SystemHelper::GetLastTrimLevel() const {
  ScopedJNIAttach jni_scope(vm_);
  CALL_STATIC_VOID_METHOD(true, activity_mng_cls_, get_memory_state_mid_, running_process_info_obj_);
  return GET_INT_FIELD(true, running_process_info_obj_, last_trim_level_fid_);
}

long SystemHelper::GetFreeMemory() const {
  ScopedJNIAttach jni_scope(vm_);
  jclass android_os_process_class = FIND_CLASS(true, "android/os/Process");
  jmethodID get_free_memory_mid = GET_STATIC_METHOD_ID(true, android_os_process_class, "getFreeMemory", "()J");
  return CALL_STATIC_LONG_METHOD(true, android_os_process_class, get_free_memory_mid);
}

int SystemHelper::GetUnicodeChar(int event_type, int key_code, int meta_state) const {
  ScopedJNIAttach jni_scope(vm_);
  jclass class_key_event = jni_scope->FindClass("android/view/KeyEvent");
  int unicode_key = -1;
  if (meta_state == 0) {
    static jmethodID method_get_unicode_char1 = GET_METHOD_ID(true, class_key_event, "getUnicodeChar", "()I");
    static jmethodID event_constructor1 = GET_METHOD_ID(true, class_key_event, "<init>", "(II)V");
    jobject event_obj = jni_scope->NewObject(class_key_event, event_constructor1, event_type, key_code);
    unicode_key = CALL_INT_METHOD(true, event_obj, method_get_unicode_char1);
  } else {
    static jmethodID method_get_unicode_char2 = GET_METHOD_ID(true, class_key_event, "getUnicodeChar", "(I)I");
    static jmethodID event_constructor2 = GET_METHOD_ID(true, class_key_event, "<init>", "(II)V");
    jobject event_obj = jni_scope->NewObject(class_key_event, event_constructor2, event_type, key_code);
    unicode_key = CALL_INT_METHOD(true, event_obj, method_get_unicode_char2, meta_state);
  }
  jni_scope->DeleteLocalRef(class_key_event);
  return unicode_key;
}

bool SystemHelper::IsWifiOn() const {
  ScopedJNIAttach jni_scope(vm_);
  jfieldID wifi_service_field = GET_STATIC_FIELD_ID(true, context_, "WIFI_SERVICE", "Ljava/lang/String;");
  jstring jstr = (jstring)GET_STATIC_OBJECT_FIELD(true, context_, wifi_service_field);
  jmethodID get_system_service_id = GET_METHOD_ID(true, context_, "getSystemService",
                                                  "(Ljava/lang/String;)Ljava/lang/Object;");
  jobject wifimgr = CALL_OBJECT_METHOD(true, native_activity_, get_system_service_id, jstr);

  jclass wifi_mgr_cls = FIND_CLASS(true, "android/net/wifi/WifiManager");
  jmethodID is_wifi_enabled = GET_METHOD_ID(true, wifi_mgr_cls, "isWifiEnabled", "()Z");
  jboolean is_wifi_on = CALL_BOOLEAN_METHOD(true, wifimgr, is_wifi_enabled);

  jni_scope->DeleteLocalRef(jstr);
  jni_scope->DeleteLocalRef(wifimgr);
  jni_scope->DeleteLocalRef(wifi_mgr_cls);
  return (is_wifi_on == true);
}

bool SystemHelper::IsNetworkConnected() const {
  ScopedJNIAttach jni_scope(vm_);
  jfieldID connectivityServiceField = GET_STATIC_FIELD_ID(true, context_, "CONNECTIVITY_SERVICE", "Ljava/lang/String;");
  jstring jstr = (jstring)GET_STATIC_OBJECT_FIELD(true, context_, connectivityServiceField);
  jmethodID get_system_service_id = GET_METHOD_ID(true, context_, "getSystemService",
                                                  "(Ljava/lang/String;)Ljava/lang/Object;");
  jobject connmgr = CALL_OBJECT_METHOD(true, native_activity_, get_system_service_id, jstr);

  jmethodID get_active_network = GET_METHOD_ID(true, network_cls_, "getActiveNetwork", "()Landroid/net/Network;");
  jobject network = CALL_OBJECT_METHOD(true, connmgr, get_active_network);

  jni_scope->DeleteLocalRef(jstr);
  jni_scope->DeleteLocalRef(connmgr);
  return (network != NULL);  // No active network getActiveNetwork call returns NULL
}

bool SystemHelper::IsInternetAvailable() const {
  ScopedJNIAttach jni_scope(vm_);
  jfieldID connectivityServiceField = GET_STATIC_FIELD_ID(true, context_, "CONNECTIVITY_SERVICE", "Ljava/lang/String;");
  jstring jstr = (jstring)GET_STATIC_OBJECT_FIELD(true, context_, connectivityServiceField);
  jmethodID get_system_service_id = GET_METHOD_ID(true, context_, "getSystemService",
                                                  "(Ljava/lang/String;)Ljava/lang/Object;");
  jobject connmgr = CALL_OBJECT_METHOD(true, native_activity_, get_system_service_id, jstr);

  jmethodID get_active_network = GET_METHOD_ID(true, network_cls_, "getActiveNetwork", "()Landroid/net/Network;");
  jobject network = CALL_OBJECT_METHOD(true, connmgr, get_active_network);
  jmethodID get_network_capabilities = GET_METHOD_ID(true, network_cls_, "getNetworkCapabilities",
                                                     "(Landroid/net/Network;)Landroid/net/NetworkCapabilities;");
  jobject capabilities = CALL_OBJECT_METHOD(true, connmgr, get_network_capabilities, network);

  if (capabilities == NULL) {
    jni_scope->DeleteLocalRef(jstr);
    jni_scope->DeleteLocalRef(connmgr);
    jni_scope->DeleteLocalRef(network);
    return false;  // No active network getActiveNetwork call returns NULL
  }

  jclass network_capabilities_cls = FIND_CLASS(true, "android/net/NetworkCapabilities");
  jmethodID hascapability = GET_METHOD_ID(true, network_capabilities_cls, "hasCapability", "(I)Z");
  jboolean networkvalid = CALL_BOOLEAN_METHOD(true, capabilities, hascapability,
                                              16);  // NET_CAPABILITY_VALIDATED value 16
  jboolean networkinternet = CALL_BOOLEAN_METHOD(true, capabilities, hascapability,
                                                 12);  // NET_CAPABILITY_INTERNET value 12

  jni_scope->DeleteLocalRef(jstr);
  jni_scope->DeleteLocalRef(connmgr);
  jni_scope->DeleteLocalRef(network);
  jni_scope->DeleteLocalRef(capabilities);
  jni_scope->DeleteLocalRef(network_capabilities_cls);
  return (networkinternet && networkvalid);
}

}  // namespace app_framework
}  // namespace ml
