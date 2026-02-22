add_library(hesiod_platform INTERFACE)

# Linux
if(UNIX AND NOT APPLE)
  message(STATUS "Platform: Linux")
  target_compile_definitions(hesiod_platform INTERFACE HSD_OS_LINUX)

# macOS
elseif(APPLE)
  message(STATUS "Platform: macOS")
  target_compile_definitions(hesiod_platform INTERFACE HSD_OS_MACOS)

  # macOS requires linking against system frameworks
  find_library(COCOA_FRAMEWORK Cocoa)
  find_library(OPENGL_FRAMEWORK OpenGL)
  find_library(OPENCL_FRAMEWORK OpenCL)
  find_library(IOKIT_FRAMEWORK IOKit)

  if(OPENGL_FRAMEWORK)
    target_link_libraries(hesiod_platform INTERFACE ${OPENGL_FRAMEWORK})
  endif()

  if(OPENCL_FRAMEWORK)
    target_link_libraries(hesiod_platform INTERFACE ${OPENCL_FRAMEWORK})
  endif()

  # Set minimum macOS deployment target for Metal-backed OpenCL and modern OpenGL
  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS deployment target" FORCE)
  endif()

  # Support both x86_64 and arm64 (Universal Binary) if not already set
  if(NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "${CMAKE_SYSTEM_PROCESSOR}" CACHE STRING "macOS architectures" FORCE)
  endif()

# Windows
elseif(WIN32)
  message(STATUS "Platform: Windows")

# Unsupported platforms
else()
  message(
    FATAL_ERROR "Unsupported platform. Only Linux, macOS, and Windows are supported.")
endif()
