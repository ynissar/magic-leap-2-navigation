# Copyright 2021 (c) Magic Leap Inc.

#[=======================================================================[.rst:
FindMagicLeapAppFramework.cmake
-------

sets up a app_framework library target.

#]=======================================================================]

find_package(MagicLeap REQUIRED)

if (MagicLeapAppFramework_FOUND)
  return()
endif()

find_package(MagicLeapNativeAppGlue REQUIRED)
find_package(MagicLeapOpenGL REQUIRED)

if ( CMAKE_BUILD_TYPE STREQUAL "" )
    SET(CMAKE_BUILD_TYPE "RELEASE")
endif()

include(af_buildpaths)

include(FetchContent)
FetchContent_Declare(
  app_framework_lib
  SOURCE_DIR   "${CMAKE_CURRENT_LIST_DIR}/../"
  # SUBBUILD_DIR "${MagicLeapAppFramework_SUB_CACHE}/app_framework"
  # BINARY_DIR   "${MagicLeapAppFramework_BIN_CACHE}/app_framework"
)
FetchContent_MakeAvailable(app_framework_lib)

set(MagicLeapAppFramework_FOUND TRUE)

set(CMAKE_DEBUG_POSTFIX "")