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
#include <string>
#include <unordered_map>
#include <vector>

namespace ml {
namespace app_framework {

class SystemHelper {
public:
  SystemHelper(struct android_app *app);
  ~SystemHelper();

  // make sure the public getter methods below are called from the android_main() thread
  int GetVersionCode() const;
  const std::string &GetPackageName() const;
  const std::string &GetVersionName() const;
  const std::string &GetApplicationInstallDir() const;
  const std::string &GetApplicationWritableDir() const;
  const std::string &GetExternalFilesDir() const;
  const std::string &GetISO3Language() const;
  bool CheckApplicationPermission(const std::string &permission_name) const;
  void RequestApplicationPermission(const std::string &permission_name) const;
  void RequestApplicationPermissions(const std::vector<std::string> &permissions) const;
  long GetAvailableDiskBytes() const;
  long GetAvailableExternalBytes() const;
  std::unordered_map<std::string, std::string> &GetIntentExtras() const;
  int GetComputePackBatteryLevel() const;
  int GetComputePackBatteryStatus() const;
  float GetComputePackBatteryTemperature() const;
  long GetComputePackChargeTimeRemaining() const;
  int GetControllerBatteryLevel() const;
  int GetControllerBatteryStatus() const;
  bool IsPowerBankPresent() const;
  int GetPowerBankBatteryLevel() const;
  long GetFreeMemory() const;
  int GetLastTrimLevel() const;
  long GetTotalDiskBytes() const;
  long GetTotalExternalBytes() const;
  int GetUnicodeChar(int event_type, int key_code, int meta_state) const;
  bool IsControllerPresent() const;
  bool IsInteractive() const;
  bool IsWifiOn() const;
  bool IsNetworkConnected() const;
  bool IsInternetAvailable() const;

private:
  mutable int version_code_;
  mutable std::string package_name_;
  mutable std::string app_writable_dir_;
  mutable std::string external_files_dir_;
  mutable std::string app_install_dir_;
  mutable std::string version_name_;
  mutable std::string language_;
  mutable std::unordered_map<std::string, std::string> intent_extras_;

#if ML_LUMIN
  JavaVM *vm_;
  jclass activity_mng_cls_;
  jclass context_;
  jclass network_cls_;
  jclass pkg_mng_cls_;

  jfieldID last_trim_level_fid_;

  jmethodID get_batt_property_mid_;
  jmethodID get_batt_charge_time_mid_;
  jmethodID get_avail_disk_bytes_mid_;
  jmethodID get_avail_external_bytes_mid_;
  jmethodID get_memory_state_mid_;
  jmethodID get_total_disk_bytes_mid_;
  jmethodID get_total_external_bytes_mid_;
  jmethodID registerReceiver_mid_;

  jobject activity_manager_;
  jobject batt_mng_obj_;
  jobject batt_changed_int_obj_;
  jobject controller_intent_filter_;
  jobject powerbank_intent_filter_;
  jobject native_activity_;
  jobject statsf_external_storage_obj;
  jobject statsf_internal_storage_obj;

  mutable jobject pwr_mgr_;
  mutable jobject running_process_info_obj_;
  mutable jmethodID is_interactive_mid_;
  mutable jobject pkg_info_obj_;
  mutable jobject app_info_obj_;

  jobject GetPackageInfo() const;
  jobject GetAppInfo() const;
#else
  mutable std::vector<std::string> host_args_;
#endif
};

}  // namespace app_framework
}  // namespace ml
