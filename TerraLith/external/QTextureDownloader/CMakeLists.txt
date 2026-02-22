cmake_minimum_required(VERSION 3.12)
project(qterrain_downloader-root VERSION 0.0.0)

# --- global setup

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

if(UNIX AND NOT APPLE)
  message(STATUS "Configuring for LINUX system...")
  add_definitions("-DQTD_OS_LINUX")
else()
  message(STATUS "NOT configuring for LINUX system...")
endif()

# --- release type

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting default build type to Release")
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Choose the type of build." FORCE)
endif()

message(STATUS "Release type: ${CMAKE_BUILD_TYPE}")

add_compile_definitions($<$<CONFIG:Debug>:DEBUG_BUILD>
                        $<$<CONFIG:Release>:RELEASE_BUILD>)

set(tmp_qt_logging_rules
    "qt.widgets.gestures.debug=false;qt.qpa.*=false;qt.pointer.*=false")
string(REPLACE ";" "\\;" tmp_qt_logging_rules_escaped "${qt_logging_rules}")
add_compile_definitions(
  QTD_QPUTENV_QT_LOGGING_RULES="\"${tmp_qt_logging_rules_escaped}\"")

# --- compiler options

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # MSVC compiler options
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} /W4 /Od /D M_PI=3.14159265358979323846")
else()
  # GNU compiler options

  # Common to all configs
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -Ofast -ffast-math -Wall -Wextra -Wno-dangling-reference -pthread -Wno-deprecated-declarations"
  )

  # Add per-configuration flags
  set(CMAKE_CXX_FLAGS_DEBUG
      "${CMAKE_CXX_FLAGS_DEBUG} -g -DLOG_LEVEL=3 -fno-omit-frame-pointer \
    -Wpedantic -Wno-psabi -Wno-unused-parameter \
    -Wcast-align -Wcast-qual  \
    -Wnull-dereference -Wnon-virtual-dtor -Woverloaded-virtual \
    -Wzero-as-null-pointer-constant -Wduplicated-cond \
    -Wduplicated-branches -Wlogical-op -Wuseless-cast \
    ")

  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DLOG_LEVEL=3")
endif()

# --- dependencies

find_package(nlohmann_json 3.11 REQUIRED)
find_package(spdlog REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

add_subdirectory(QTextureDownloader)
add_subdirectory(tests)
