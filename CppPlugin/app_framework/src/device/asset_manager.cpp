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

#include <app_framework/asset_manager.h>
#include <stdexcept>

#include <android/asset_manager.h>

using namespace ml;

// https://developer.android.com/ndk/reference/group/asset#group___asset_1ga0037ce3c10a591fe632f34c1aa62955c
class AndroidAsset : public IAsset {
public:
  AndroidAsset(AAsset *asset) : asset_(asset) {}
  ~AndroidAsset() {
    Close();
  }

  virtual void Close() {
    if (asset_ != nullptr) {
      AAsset_close(asset_);
      asset_ = nullptr;
    }
  }
  virtual size_t Read(void *buffer, size_t count) {
    if (!asset_) {
      throw std::runtime_error("asset closed before calling Read");
    }
    return AAsset_read(asset_, buffer, count);
  }

  virtual int64_t Seek(int64_t offset, int whence) {
    if (!asset_) {
      throw std::runtime_error("asset closed before calling Seek");
    }
    return AAsset_seek64(asset_, offset, whence);
  }

  virtual int64_t Length() {
    if (!asset_) {
      throw std::runtime_error("asset closed before calling Length");
    }
    return AAsset_getLength64(asset_);
  }

  virtual const void *GetBuffer() {
    if (!asset_) {
      throw std::runtime_error("asset closed before calling GetBuffer");
    }
    return AAsset_getBuffer(asset_);
  }

  virtual int OpenFileDescriptor(int64_t &start, int64_t &length) {
    if (!asset_) {
      throw std::runtime_error("asset closed before calling OpenFileDescriptor");
    }
    return AAsset_openFileDescriptor64(asset_, &start, &length);
  }

private:
  AAsset *asset_;
};

class AndroidAssetManager : public IAssetManager {
  AAssetManager *asset_manager_;

public:
  AndroidAssetManager(ANativeActivity *activity) : asset_manager_(activity->assetManager) {}

  virtual IAssetPtr Open(const char *filename) {
    AAsset *asset = AAssetManager_open(asset_manager_, filename, 0);
    if (asset == nullptr) {
      return nullptr;
    }

    return std::make_shared<AndroidAsset>(asset);
  }
};

IAsset::~IAsset() {}

IAssetManager::~IAssetManager() {}

IAssetManagerPtr IAssetManager::Factory(struct android_app *app) {
  return std::make_shared<AndroidAssetManager>(app->activity);
}
