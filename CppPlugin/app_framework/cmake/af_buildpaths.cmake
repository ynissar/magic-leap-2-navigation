# Copyright 2021 (c) Magic Leap Inc.

#[=======================================================================[.rst:
af_buildpaths.cmake
-------

sets up the build target paths so that
a single build of app_framework can be shared
amongst all samples & tests. (per target & build type.)

#]=======================================================================]


if ( "${ML_AF_BUILD_CACHE}" STREQUAL "" )
    SET(ML_AF_BUILD_CACHE "${CMAKE_CURRENT_LIST_DIR}/../build/")
endif()

SET(MagicLeapAppFramework_SRC_CACHE "${ML_AF_BUILD_CACHE}/src/")
# SET(MagicLeapAppFramework_SUB_CACHE "${ML_AF_BUILD_CACHE}/sub/${ML_TARGET}/${CMAKE_BUILD_TYPE}/")
# SET(MagicLeapAppFramework_BIN_CACHE "${ML_AF_BUILD_CACHE}/bin/${ML_TARGET}/${CMAKE_BUILD_TYPE}/")
