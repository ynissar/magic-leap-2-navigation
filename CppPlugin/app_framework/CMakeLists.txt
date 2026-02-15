cmake_minimum_required(VERSION 3.18.1 )

file(TO_CMAKE_PATH "$ENV{MLSDK}" MLSDK)

list(APPEND CMAKE_MODULE_PATH "${MLSDK}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

project(app_framework)

find_package(MagicLeap REQUIRED)

set(CMAKE_DEBUG_POSTFIX "")

# -----------------------------------------------------------------------------
# Setup all the external dependencies
include (setupExternals)

# -----------------------------------------------------------------------------
# Build the app_framework library
if(LUMIN)
  add_definitions(-DML_LUMIN)
  find_path(GLES3_INCLUDE_DIR GLES3/gl3.h HINTS "${ANDROID_NDK}")
  find_library(GLES3_LIBRARY libGLESv3.so HINTS "${GLES3_INCLUDE_DIR}/../lib")
  set(GRAPHICS_CONTEXT src/device/graphics_context.cpp)
  set(WEBREQUEST_CONTEXT src/device/web_request.cpp)
  set(SYSTEM_HELPER src/device/system_helper.cpp)
  set(INTENT_HELPER src/device/intent_helper.cpp)
  set(ASSET_MANAGER src/device/asset_manager.cpp)
  set(HOST_LIBS "")
  set(NATIVE_APP_GLUE_DIRS "${ANDROID_NDK}/sources/android/native_app_glue")
else()
  find_package(OpenGL REQUIRED)
  set(GLES3_LIBRARY "OpenGL::GL")
  set(GRAPHICS_CONTEXT src/host/graphics_context.cpp)
  set(WEBREQUEST_CONTEXT src/host/web_request.cpp)
  set(SYSTEM_HELPER src/host/system_helper.cpp)
  set(INTENT_HELPER src/host/intent_helper.cpp)
  set(ASSET_MANAGER src/host/asset_manager.cpp)
  set(HOST_LIBS glfw)
  set(NATIVE_APP_GLUE_DIRS "${MLSDK}/zi_android_sdk/include" "${MLSDK}/zi_android_sdk/native_app_glue")

endif()

set (SRCS
  src/application.cpp
  src/cli_args_parser.cpp
  src/convert.cpp
  src/node.cpp
  src/pose.cpp
  src/gui.cpp
  src/imgui_console.cpp
  src/render/program.cpp
  src/render/geometry_program.cpp
  src/render/material.cpp
  src/render/renderer.cpp
  src/render/buffer.cpp
  src/render/variable.cpp
  src/render/mesh.cpp
  src/render/texture.cpp
  src/render/render_target.cpp
  src/registry.cpp
  src/resource_pool.cpp
  src/input/ml_input_handler.cpp
  src/input/input_command_handler.cpp
  src/input/keyboard.cpp
  ${GRAPHICS_CONTEXT}
  ${WEBREQUEST_CONTEXT}
  ${SYSTEM_HELPER}
  ${INTENT_HELPER}
  ${ASSET_MANAGER})

add_library(app_framework ${SRCS})

target_include_directories(app_framework
  PUBLIC
  include
  PRIVATE
  ${NATIVE_APP_GLUE_DIRS}
  include/app_framework/render
  ${GLFW_INCLUDE_DIRS}
  )


target_link_directories(app_framework PRIVATE ${ML_SDK_LIBRARY_PATH})



if(ML_TARGET STREQUAL ml2)
  list(APPEND ML_APP_FRAMEWORK_LIBS z)
elseif(ML_TARGET STREQUAL osx)
  list(APPEND ML_APP_FRAMEWORK_LIBS stdc++ zlibstatic zi.magicleap glfw "-framework Cocoa" "-framework IOKit" "-framework CoreFoundation" "-framework CoreVideo")
elseif(ML_TARGET STREQUAL linux)
  list(APPEND ML_APP_FRAMEWORK_LIBS stdc++ zlibstatic zi.magicleap glfw X11 Xrandr Xcursor Xinerama Xxf86vm)
elseif(ML_TARGET STREQUAL win)
  if(MSVC_TOOLSET_VERSION)
      list(APPEND ML_APP_FRAMEWORK_LIBS zlibstatic shlwapi zi.magicleap glfw)
  else()
      list(APPEND ML_APP_FRAMEWORK_LIBS zlibstatic shlwapi zi.magicleap glfw)
  endif()
else()
message(FATAL_ERROR "Unsupported platform")
endif()

# Make sure that 3rd party apps are built with -Wno-error
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(assimp PRIVATE -WX-)
  target_compile_options(imgui PRIVATE -WX-)
  target_compile_options(stb PRIVATE -WX-)
  target_compile_options(glad PRIVATE -WX-)
else()
  target_compile_options(assimp PRIVATE -Wno-error)
  target_compile_options(imgui PRIVATE -Wno-error)
  target_compile_options(stb PRIVATE -Wno-error)
  target_compile_options(glad PRIVATE -Wno-error)
endif()

target_link_libraries(app_framework
  PUBLIC
  android
  native_app_glue
  gl.magicleap
  ML::input
  ML::graphics
  ML::perception
  ${GLES3_LIBRARY}
  gflags
  ${ML_APP_FRAMEWORK_LIBS}
  assimp
  glm
  imgui
  stb
  glad
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(app_framework PRIVATE -WX -W2)
  target_compile_features(app_framework PUBLIC cxx_std_17)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(app_framework PRIVATE -Werror -Wall -Wextra -fPIC -Wno-error=stringop-overflow)
  target_compile_features(app_framework PUBLIC cxx_std_17  )
  target_compile_options(app_framework PUBLIC  -Wno-deprecated-declarations)
else()
  target_compile_options(app_framework PRIVATE -Werror -Wall -Wextra)
  target_compile_features(app_framework PUBLIC cxx_std_17)
  target_compile_options(app_framework PUBLIC  -Wno-deprecated)
endif()

add_library(ML::app_framework ALIAS app_framework)


# -----------------------------------------------------------------------------
# Install app_framework
install(DIRECTORY "include" DESTINATION ".")
install (TARGETS app_framework
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
