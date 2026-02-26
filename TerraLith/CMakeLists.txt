cmake_minimum_required(VERSION 3.12)
project(hesiod-root VERSION 0.6.0)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# Version info
add_compile_definitions(
  HESIOD_VERSION_MAJOR=${hesiod-root_VERSION_MAJOR}
  HESIOD_VERSION_MINOR=${hesiod-root_VERSION_MINOR}
  HESIOD_VERSION_PATCH=${hesiod-root_VERSION_PATCH})

# ------------------------------
# Options
# ------------------------------
if(NOT APPLE)
  option(HESIOD_ENABLE_GENERATE_APP_IMAGE "Enable AppImage generation" ON)
endif()

option(HESIOD_MINIMAL_NODE_SET
       "Use a minimal set of nodes (ADVANCED DEV ONLY!!!)" OFF)

# Option to enable LTO and unused-function detection
option(HESIOD_ENABLE_LTO "Enable LTO and unused-function detection" OFF)
option(HESIOD_UNUSED_FUNCTIONS "Detect unused static functions" OFF)

# Vulkan compute support
option(HESIOD_ENABLE_VULKAN "Enable Vulkan compute support" ON)

# ------------------------------
# Modules
# ------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(HesiodCompilerOptions)
include(HesiodPlatform)
include(HesiodQtLoggingRules)
include(HesiodGitVersion)

# ------------------------------
# Dependencies
# ------------------------------
find_package(spdlog REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets)
find_package(nlohmann_json REQUIRED)

# --- Vulkan configuration ---
if(HESIOD_ENABLE_VULKAN)
  find_package(Vulkan REQUIRED COMPONENTS glslc)
  message(STATUS "Vulkan found: ${Vulkan_LIBRARY}")
  message(STATUS "Vulkan glslc: ${Vulkan_GLSLC_EXECUTABLE}")
  add_compile_definitions(HESIOD_HAS_VULKAN)
endif()

# --- OpenCL configuration ---
if(APPLE)
  # macOS: OpenCL is deprecated but still available via the system framework.
  # Default GPU mode OFF on macOS since OpenCL is deprecated and may be slower
  # than CPU on some Apple Silicon configurations.
  add_compile_definitions(
    CL_HPP_MINIMUM_OPENCL_VERSION=120 CL_HPP_TARGET_OPENCL_VERSION=120
    HSD_DEFAULT_GPU_MODE=false
    CL_SILENCE_DEPRECATION)
else()
  add_compile_definitions(
    CL_HPP_MINIMUM_OPENCL_VERSION=120 CL_HPP_TARGET_OPENCL_VERSION=120
    HSD_DEFAULT_GPU_MODE=true)
endif()

# ------------------------------
# Subdirectories
# ------------------------------
add_subdirectory(external)
add_subdirectory(Hesiod)

if(NOT APPLE AND HESIOD_ENABLE_GENERATE_APP_IMAGE)
  add_subdirectory(scripts/generate_appimage)
endif()
