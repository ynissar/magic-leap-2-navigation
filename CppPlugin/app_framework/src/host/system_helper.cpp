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
#include <ml_zi_permissions.h>

static const std::string kEmptyStr = "";
// For App Sim (aka ZI): use current working directory for some cases.
static const std::string kCurrentDir = "./";
static const std::string kCurrentLang = "eng";

namespace ml {
namespace app_framework {

SystemHelper::SystemHelper(struct android_app *app) {
  version_code_ = -1;
  package_name_ = "com.magicleap.zi";

  // Make a copy of the command line arguments
  if (app->argc > 0 && app->argv != nullptr) {
    host_args_.insert(host_args_.end(), &app->argv[0], &app->argv[app->argc]);
  }

  if (app->applicationInstallDir) {
    app_install_dir_ = app->applicationInstallDir;
  }
  if (app->applicationWritableDir) {
    app_writable_dir_ = app->applicationWritableDir;
  }
  if (app->externalFilesDir) {
    external_files_dir_ = app->externalFilesDir;
  }

  MLZIPermissionsStart();
}

SystemHelper::~SystemHelper() {
  MLZIPermissionsStop();
}

const std::string &SystemHelper::GetPackageName() const {
  return package_name_;
}

int SystemHelper::GetVersionCode() const {
  return 0;
}

const std::string &SystemHelper::GetVersionName() const {
  // App Sim code goes here
  return kEmptyStr;
}

const std::string &SystemHelper::GetApplicationInstallDir() const {
  return app_install_dir_;
}

const std::string &SystemHelper::GetApplicationWritableDir() const {
  return (app_writable_dir_.empty()) ? kCurrentDir : app_writable_dir_;
}

const std::string &SystemHelper::GetExternalFilesDir() const {
  return (external_files_dir_.empty()) ? kCurrentDir : external_files_dir_;
}

const std::string &SystemHelper::GetISO3Language() const {
  return (language_.empty()) ? kCurrentLang : language_;
}

long SystemHelper::GetFreeMemory() const {
  // App Sim code goes here
  return 0UL;
}

long SystemHelper::GetTotalDiskBytes() const {
  // App Sim code goes here
  return 0UL;
}

long SystemHelper::GetAvailableExternalBytes() const {
  // App Sim code goes here
  return 0UL;
}
long SystemHelper::GetTotalExternalBytes() const {
  // App Sim code goes here
  return 0UL;
}

long SystemHelper::GetAvailableDiskBytes() const {
  // App Sim code goes here
  return 0UL;
}

bool SystemHelper::IsInteractive() const {
  // App Sim code goes here
  return true;
}

bool SystemHelper::CheckApplicationPermission(const std::string &permission_name) const {
  MLResult result = MLZIPermissionsIsGranted(permission_name.c_str());
  return result == MLResult_Ok;
}

void SystemHelper::RequestApplicationPermission(const std::string &permission_name) const {
  MLZIPermissionsRequest(permission_name.c_str());
}

void SystemHelper::RequestApplicationPermissions(const std::vector<std::string> &permissions) const {
  for (auto &permission : permissions) {
    RequestApplicationPermission(permission);
  }
}

std::unordered_map<std::string, std::string> &SystemHelper::GetIntentExtras() const {
  // Process as key/ value pairs
  intent_extras_.clear();
  for (size_t i = 0; i < host_args_.size(); i += 2) {
    intent_extras_[host_args_[i]] = {};
    if ((i + 1) < host_args_.size()) {
      intent_extras_[host_args_[i]] = host_args_[i + 1];
    }
  }

  return intent_extras_;
}

int SystemHelper::GetComputePackBatteryLevel() const {
  // App Sim code goes here
  return 0;
}

int SystemHelper::GetLastTrimLevel() const {
  // App Sim code goes here
  return 0;
}

int SystemHelper::GetComputePackBatteryStatus() const {
  // App Sim code goes here
  return 0;
}

float SystemHelper::GetComputePackBatteryTemperature() const {
  // App Sim code goes here
  return 0.0f;
}

long SystemHelper::GetComputePackChargeTimeRemaining() const {
  // App Sim code goes here
  return -1;
}

bool SystemHelper::IsControllerPresent() const {
  // Not implemented for App Sim / appsim
  return false;
}

int SystemHelper::GetControllerBatteryLevel() const {
  // Not implemented for App Sim / appsim
  return 0;
}

int SystemHelper::GetControllerBatteryStatus() const {
  // Not implemented for App Sim / appsim
  return 0;
}

bool SystemHelper::IsPowerBankPresent() const {
  // Not implemented for App Sim / appsim
  return false;
}

int SystemHelper::GetPowerBankBatteryLevel() const {
  // Not implemented for App Sim / appsim
  return 0;
}

int SystemHelper::GetUnicodeChar(int event_type, int key_code, int meta_state) const {
  // App Sim code goes here
  (void)event_type;
  (void)key_code;
  (void)meta_state;
  return 0;
}

bool SystemHelper::IsWifiOn() const {
  return false;
}

bool SystemHelper::IsNetworkConnected() const {
  return false;
}

bool SystemHelper::IsInternetAvailable() const {
  return false;
}

}  // namespace app_framework
}  // namespace ml
