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
#include <app_framework/logging.h>

#include <codecvt>
#include <fstream>
#include <iostream>
#include <locale>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <shlwapi.h>
typedef std::wstring_convert<std::codecvt_utf8_utf16<wchar_t, 0x10ffff, std::little_endian>> Utf8Utf16Converter;
#endif

using namespace ml;

#if _WIN32

static Utf8Utf16Converter &GetUtf8Utf16StrConverter() {
  static Utf8Utf16Converter converter;
  return converter;
}

static std::wstring ConvertToWideString(const std::string &utf8Str) {
  try {
    return GetUtf8Utf16StrConverter().from_bytes(utf8Str);
  } catch (const std::range_error &e) {
    ALOGE("Failed to convert UTF-8 string to wide");
    // don't be clever, let programmer fix it
    std::vector<wchar_t> wcs;
    wcs.reserve(utf8Str.size());
    for (auto ch : utf8Str) {
      wcs.push_back(uint8_t(ch));
    }
    return std::wstring(wcs.data(), wcs.size());
  }
}

static std::wstring ConvertToFsPath(const std::string &utf8Path) {
  return ConvertToWideString(utf8Path);
}

static FILE *FopenUtf8(const std::string &path, const char *mode) {
  return _wfopen(ConvertToFsPath(path).c_str(), ConvertToWideString(mode).c_str());
}

#else

static FILE *FopenUtf8(const std::string &path, const char *mode) {
  return fopen(path.c_str(), mode);
}

#endif

class HostAsset : public IAsset {
public:
  HostAsset(const char *filename) {
    file_ = FopenUtf8(filename, "rb");  // "b" is critical on Windows, else Assimp fails to read FBX files
  }

  ~HostAsset() {
    Close();
  }

  virtual void Close() {
    if (file_) {
      fclose(file_);
      file_ = nullptr;
    }
  }

  virtual size_t Read(void *buffer, size_t count) {
    if (!file_) {
      throw std::runtime_error("asset closed before calling Read");
    }

    return fread(buffer, count, 1, file_);
  }

  virtual int64_t Seek(int64_t offset, int whence) {
    if (!file_) {
      throw std::runtime_error("asset closed before calling Seek");
    }

    return fseek(file_, offset, whence);
  }

  virtual int64_t Length() {
    if (!file_) {
      throw std::runtime_error("asset closed before calling Length");
    }

    fseek(file_, 0, SEEK_END);
    int64_t length = ftell(file_);
    rewind(file_);
    return length;
  }

  virtual const void *GetBuffer() {
    if (!file_) {
      throw std::runtime_error("asset closed before calling GetBuffer");
    }

    auto length = Length();  // also rewinds
    buffer_.resize(length);
    size_t read = fread(buffer_.data(), 1, length, file_);
    buffer_.resize(read);

    return buffer_.data();
  }

  virtual int OpenFileDescriptor(int64_t &start, int64_t &length) {
    (void)start;
    (void)length;

    ALOGE("Not implemented for host!");
    return -1;
  }

private:
  FILE *file_{nullptr};
  std::vector<char> buffer_;
};

class HostAssetManager : public IAssetManager {
public:
  HostAssetManager() {}

  virtual IAssetPtr Open(const char *filename) {
    return std::make_shared<HostAsset>(filename);
  }
};

IAsset::~IAsset() {}

IAssetManager::~IAssetManager() {}

IAssetManagerPtr IAssetManager::Factory(struct android_app *app) {
  (void)app;
  return std::make_shared<HostAssetManager>();
}
