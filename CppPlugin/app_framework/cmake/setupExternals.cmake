if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif()

# Fix behavior of CMAKE_CXX_STANDARD when targeting macOS.
if (POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW)
endif ()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-fPIC)
endif()

find_package (Python COMPONENTS Interpreter)
set(PYTHON ${Python_EXECUTABLE})

include(af_buildpaths)

# -----------------------------------------------------------------------------
# Build and install external dependencies. If CI_BUILD environment variable is
# set then the external files are going to be downloaded from internal
# sources.

if("$ENV{CI_BUILD}" STREQUAL "1")
  message("Using CI mirror")
  file(READ external_deps.jenkins.csv data)
else()
  file(READ external_deps.csv data)
endif()
string(REPLACE "\n" ";" data "${data}")
foreach(line IN LISTS data)
  string(REPLACE "," ";" line "${line}")
  list(LENGTH line length)
  if("${length}" EQUAL "4")
    list(GET line 0 name)
    list(GET line 1 zip)
    list(GET line 2 url)
    list(GET line 3 dir)
    get_filename_component(zip ${zip} NAME)
    set(EXTERNAL_${name}    ${name} CACHE INTERNAL "")
    set(EXTERNAL_${name}_ZIP ${zip} CACHE INTERNAL "")
    set(EXTERNAL_${name}_URL ${url} CACHE INTERNAL "")
    set(EXTERNAL_${name}_DIR ${dir} CACHE INTERNAL "")
  endif()
endforeach()

include(FetchContent)

# ----------------------------------------------------------------------------
# zlib, only required for host building.
#
# patch created with
# diff -ur zlib-orig/ zlib/ > ../../cmake/zlib.patch
# #
# if(NOT LUMIN)
# FetchContent_Declare(
#   ${EXTERNAL_zlib}
#   URL ${EXTERNAL_zlib_URL}
#   SOURCE_DIR "${MagicLeapAppFramework_SRC_CACHE}/${EXTERNAL_zlib}/"
#   # SUBBUILD_DIR "${MagicLeapAppFramework_SUB_CACHE}/${EXTERNAL_zlib}"
#   # BINARY_DIR   "${MagicLeapAppFramework_BIN_CACHE}/${EXTERNAL_zlib}"
#   PATCH_COMMAND ${PYTHON} ${CMAKE_CURRENT_LIST_DIR}/patch.py -p1 -d <SOURCE_DIR>/ ${CMAKE_CURRENT_LIST_DIR}/zlib.patch
#   QUIET)

# FetchContent_MakeAvailable(${EXTERNAL_zlib})

# endif()

# ----------------------------------------------------------------------------
# assimp
#
# patch created with
# diff -ur assimp-orig/ assimp/ > ../../cmake/assimp.patch
#
FetchContent_Declare(
  ${EXTERNAL_assimp}
  URL ${EXTERNAL_assimp_URL}
  # SOURCE_DIR "${MagicLeapAppFramework_SRC_CACHE}/${EXTERNAL_assimp}/"
  # SUBBUILD_DIR "${MagicLeapAppFramework_SUB_CACHE}/${EXTERNAL_assimp}"
  # BINARY_DIR   "${MagicLeapAppFramework_BIN_CACHE}/${EXTERNAL_assimp}"
  PATCH_COMMAND ${PYTHON} ${CMAKE_CURRENT_LIST_DIR}/patch.py -p1 -d <SOURCE_DIR>/ ${CMAKE_CURRENT_LIST_DIR}/assimp.patch
  QUIET)
FetchContent_GetProperties(${EXTERNAL_assimp})
if(NOT ${EXTERNAL_assimp}_POPULATED)
  FetchContent_Populate(${EXTERNAL_assimp})
  set(INJECT_DEBUG_POSTFIX OFF CACHE BOOL "")
  set(BUILD_SHARED_LIBS FALSE CACHE BOOL "")
  set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
  set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "")
  set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
  set(HUNTER_ENABLED OFF CACHE BOOL "")
  if(LUMIN)
    set(ASSIMP_BUILD_ZLIB OFF CACHE BOOL "")
  else()
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "")
    set(ZLIB_LIBRARIES "zlibstatic" CACHE STRING "")
  endif()
  set(SYSTEM_IRRXML OFF CACHE BOOL "")
  set(ASSIMP_NO_EXPORT ON CACHE BOOL "")
  set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT FALSE CACHE BOOL "")
  set(ASSIMP_BUILD_OBJ_IMPORTER TRUE CACHE BOOL "")
  set(ASSIMP_BUILD_FBX_IMPORTER TRUE CACHE BOOL "")
  set(ASSIMP_BUILD_GLTF_IMPORTER TRUE CACHE BOOL "")
  set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "")
  add_subdirectory(${${EXTERNAL_assimp}_SOURCE_DIR} ${${EXTERNAL_assimp}_BINARY_DIR})
  # Assimp's cmake files change its install name, so here I force it to be @rpath
  set_target_properties(assimp PROPERTIES BUILD_WITH_INSTALL_NAME_DIR ON)
  set_target_properties(assimp PROPERTIES INSTALL_NAME_DIR "@rpath")
endif()
FetchContent_MakeAvailable(${EXTERNAL_assimp})
# ----------------------------------------------------------------------------
# gflags
#
# patch created with
#
# ```sh
# diff -ur gflags-src-orig/ gflags-src/ > ../../../cmake/gflags.patch
# ```

FetchContent_Declare(${EXTERNAL_gflags}
  URL ${EXTERNAL_gflags_URL}
  QUIET
  # SOURCE_DIR   "${MagicLeapAppFramework_SRC_CACHE}/${EXTERNAL_gflags}/"
  # SUBBUILD_DIR "${MagicLeapAppFramework_SUB_CACHE}/${EXTERNAL_gflags}"
  # BINARY_DIR   "${MagicLeapAppFramework_BIN_CACHE}/${EXTERNAL_gflags}"
  PATCH_COMMAND ${PYTHON} ${CMAKE_CURRENT_LIST_DIR}/patch.py -p1 -d <SOURCE_DIR>/ ${CMAKE_CURRENT_LIST_DIR}/gflags.patch)
FetchContent_GetProperties(${EXTERNAL_gflags})
if(NOT ${EXTERNAL_gflags}_POPULATED)
  FetchContent_Populate(${EXTERNAL_gflags})
  set(GFLAGS_BUILD_SHARED_LIBS OFF CACHE BOOL "")
  set(GFLAGS_BUILD_STATIC_LIBS ON CACHE BOOL "")
  set(GFLAGS_BUILD_gflags_nothreads_LIB ON CACHE BOOL "")
  set(GFLAGS_BUILD_TESTING OFF CACHE BOOL "")
  set(GFLAGS_BUILD_PACKAGING OFF CACHE BOOL "")
  set(GFLAGS_INSTALL_HEADERS ON CACHE BOOL "")
  set(GFLAGS_INSTALL_SHARED_LIBS ON CACHE BOOL "")
  set(GFLAGS_INSTALL_STATIC_LIBS ON CACHE BOOL "")
  set(GFLAGS_INCLUDE_DIR "gflags" CACHE STRING "")
  set(GFLAGS_LIBRARY_INSTALL_DIR "lib" CACHE STRING "")
  add_subdirectory(${${EXTERNAL_gflags}_SOURCE_DIR} ${${EXTERNAL_gflags}_BINARY_DIR})
endif()
FetchContent_MakeAvailable(${EXTERNAL_gflags})

# ----------------------------------------------------------------------------
# glfw
if(NOT LUMIN)
  FetchContent_Declare(${EXTERNAL_glfw}
    URL ${EXTERNAL_glfw_URL}
    # SOURCE_DIR   "${MagicLeapAppFramework_SRC_CACHE}/${EXTERNAL_glfw}/"
    # SUBBUILD_DIR "${MagicLeapAppFramework_SUB_CACHE}/${EXTERNAL_glfw}"
    # BINARY_DIR   "${MagicLeapAppFramework_BIN_CACHE}/${EXTERNAL_glfw}"
    QUIET)
  FetchContent_GetProperties(${EXTERNAL_glfw})
  if(NOT ${EXTERNAL_glfw}_POPULATED)
    FetchContent_Populate(${EXTERNAL_glfw})
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
    add_subdirectory(${${EXTERNAL_glfw}_SOURCE_DIR} ${${EXTERNAL_glfw}_BINARY_DIR})
    set_target_properties(${EXTERNAL_glfw} PROPERTIES BUILD_WITH_INSTALL_NAME_DIR ON)
    set_target_properties(${EXTERNAL_glfw} PROPERTIES INSTALL_NAME_DIR "@rpath")
  endif()
  FetchContent_MakeAvailable(${EXTERNAL_glfw})
endif()

# ------------------------------------------------------------------------------
# png
if(LUMIN)
  FetchContent_Declare(${EXTERNAL_png}
    URL ${EXTERNAL_png_URL}
    # SOURCE_DIR "${MagicLeapAppFramework_SRC_CACHE}/${EXTERNAL_png}/"
    # SUBBUILD_DIR "${MagicLeapAppFramework_SUB_CACHE}/${EXTERNAL_png}"
    # BINARY_DIR   "${MagicLeapAppFramework_BIN_CACHE}/${EXTERNAL_png}"
    QUIET)
  FetchContent_GetProperties(${EXTERNAL_png})
  if(NOT ${EXTERNAL_png}_POPULATED)
    FetchContent_Populate(${EXTERNAL_png})
    option(PNG_SHARED false)
    option(PNG_TESTS false)
    add_subdirectory(${${EXTERNAL_png}_SOURCE_DIR} ${${EXTERNAL_png}_BINARY_DIR})
  endif()
  FetchContent_MakeAvailable(${EXTERNAL_png})
endif()


# ------------------------------------------------------------------------------
# json
FetchContent_Declare(${EXTERNAL_json}
  URL ${EXTERNAL_json_URL}
  # SOURCE_DIR "${MagicLeapAppFramework_SRC_CACHE}/${EXTERNAL_json}/"
  # SUBBUILD_DIR "${MagicLeapAppFramework_SUB_CACHE}/${EXTERNAL_json}"
  # BINARY_DIR   "${MagicLeapAppFramework_BIN_CACHE}/${EXTERNAL_json}"
  QUIET)
FetchContent_GetProperties(${EXTERNAL_json})
if(NOT ${EXTERNAL_json}_POPULATED)
  FetchContent_Populate(${EXTERNAL_json})
  set(JSON_BuildTests OFF CACHE BOOL "Override" FORCE)
  add_subdirectory(${${EXTERNAL_json}_SOURCE_DIR} ${${EXTERNAL_json}_BINARY_DIR})
endif()
FetchContent_MakeAvailable(${EXTERNAL_json})

# ------------------------------------------------------------------------------
# glm
FetchContent_Declare(${EXTERNAL_glm}
  URL ${EXTERNAL_glm_URL}
  # SOURCE_DIR "${MagicLeapAppFramework_SRC_CACHE}/${EXTERNAL_glm}/"
  # SUBBUILD_DIR "${MagicLeapAppFramework_SUB_CACHE}/${EXTERNAL_glm}"
  # BINARY_DIR   "${MagicLeapAppFramework_BIN_CACHE}/${EXTERNAL_glm}"
  QUIET
  PATCH_COMMAND ${PYTHON} ${CMAKE_CURRENT_LIST_DIR}/patch.py -p1 -d <SOURCE_DIR>/ ${CMAKE_CURRENT_LIST_DIR}/glm.patch)
FetchContent_GetProperties(${EXTERNAL_glm})
if(NOT ${EXTERNAL_glm}_POPULATED)
  FetchContent_Populate(${EXTERNAL_glm})
  set(GLM_FORCE_CTOR_INIT ON CACHE BOOL "Override" FORCE)
  set(GLM_TEST_ENABLE OFF CACHE BOOL "Override" FORCE)
  add_subdirectory(${${EXTERNAL_glm}_SOURCE_DIR} ${${EXTERNAL_glm}_BINARY_DIR})
endif()
FetchContent_MakeAvailable(${EXTERNAL_glm})

# -----------------------------------------------------------------------------
# imgui
FetchContent_Declare(${EXTERNAL_imgui}
  URL ${EXTERNAL_imgui_URL}
  # SOURCE_DIR "${MagicLeapAppFramework_SRC_CACHE}/${EXTERNAL_imgui}/"
  # SUBBUILD_DIR "${MagicLeapAppFramework_SUB_CACHE}/${EXTERNAL_imgui}"
  # BINARY_DIR   "${MagicLeapAppFramework_BIN_CACHE}/${EXTERNAL_imgui}"
  PATCH_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt.imgui <SOURCE_DIR>/CMakeLists.txt
  QUIET)
FetchContent_MakeAvailable(${EXTERNAL_imgui})

# -----------------------------------------------------------------------------
# stb
FetchContent_Declare(${EXTERNAL_stb}
  URL ${EXTERNAL_stb_URL}
  # SOURCE_DIR "${MagicLeapAppFramework_SRC_CACHE}/${EXTERNAL_stb}/"
  # SUBBUILD_DIR "${MagicLeapAppFramework_SUB_CACHE}/${EXTERNAL_stb}"
  # BINARY_DIR   "${MagicLeapAppFramework_BIN_CACHE}/${EXTERNAL_stb}"
  PATCH_COMMAND ${PYTHON} ${CMAKE_CURRENT_LIST_DIR}/patch.py -p1 -d <SOURCE_DIR>/ ${CMAKE_CURRENT_LIST_DIR}/stb.patch
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt.stb <SOURCE_DIR>/CMakeLists.txt
  QUIET)
FetchContent_MakeAvailable(${EXTERNAL_stb})

# -----------------------------------------------------------------------------
# glad
FetchContent_Declare(glad
  SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../external/glad"
  # SUBBUILD_DIR "${MagicLeapAppFramework_SUB_CACHE}/glad"
  # BINARY_DIR   "${MagicLeapAppFramework_BIN_CACHE}/glad"
  )

FetchContent_MakeAvailable(glad)
