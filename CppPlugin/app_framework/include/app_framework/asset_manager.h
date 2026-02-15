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
#include <memory>

namespace ml {

/*!
  IAsset represents an asset accessible to the application
*/
class IAsset {
public:
  virtual ~IAsset();

  /*!
    \brief Reads data from the opened asset.

    @param buffer the destination buffer.
    @param count the maximum number of bytes to copy.
    @return the number of bytes actually copied.

    @throws runtime_error if the asset was closed.
  */
  virtual size_t Read(void *buffer, size_t count) = 0;

  /*!
    \brief Moves the read pointer to requested position.

    @param offset the offset to move to
    @param whence the location to calculate the offset from,
                  constants are defined in <cstdio>

                  SEEK_SET = Beginning of file.
                  SEEK_CUR	= Current position of the file pointer.
                  SEEK_END	= End of file

    @returns actual offset.

    @throws runtime_error if the asset was closed.
  */
  virtual int64_t Seek(int64_t offset, int whence) = 0;

  /*!
    \brief Closes the file, no more access is possible
    after closing.
  */
  virtual void Close() = 0;

  /*!
    \brief Returns the length of the asset.

    @throws runtime_error if the asset was closed.
  */
  virtual int64_t Length() = 0;

  /*!
    \brief Returns a pointer to the memory of the data
    possibly mmap'd if the APK was not compressed.

    @throws runtime_error if the asset was closed.
  */
  virtual const void *GetBuffer() = 0;

  /*!
    \brief Open a file descriptor to the asset.

    returns < 0 if the asset cannot be opened as a file descriptor,
    for example when the asset was compressed in the APK. In that case
    you can update the build.gradle to avoid compression on that asset.

    start and lengths are both set by the function to the start and
    length of this asset. Note that start isn't likely to be 0!
    */
  virtual int OpenFileDescriptor(int64_t &start, int64_t &length) = 0;
};
typedef std::shared_ptr<IAsset> IAssetPtr;

class IAssetManager;
typedef std::shared_ptr<IAssetManager> IAssetManagerPtr;

/*!
  Interface class used to access assets
*/
class IAssetManager {
public:
  virtual ~IAssetManager();

  /*!
    \brief Opens an asset file for reading
    @param filename the filename to open
    @returns IAsset allowing read only access to the file or nullptr if not found.
  */
  virtual IAssetPtr Open(const char *filename) = 0;

  static IAssetManagerPtr Factory(struct android_app *app);
};

}  // namespace ml
