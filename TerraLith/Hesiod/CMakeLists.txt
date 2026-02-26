project(hesiod LANGUAGES CXX)

# ------------------------------
# Hesiod 0.6 feature options
# ------------------------------
option(HESIOD_ENABLE_SMART_PREVIEW_CACHE "Enable smart preview cache system" ON)
option(HESIOD_ENABLE_TERMINAL_LOGGING "Enable enhanced terminal logging" ON)
option(HESIOD_ENABLE_NODE_EDITOR_POLISH "Enable node editor visual polish" ON)

# ------------------------------
# Qt6 setup
# ------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOMOC_VERBOSE ON)

# ------------------------------
# Source files
# ------------------------------
file(GLOB_RECURSE HESIOD_GUI_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp)
file(GLOB_RECURSE HESIOD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/app/main.cpp)

if(HESIOD_MINIMAL_NODE_SET)
  # option for a minimal set of nodes for quick compile time when tempering with
  # node architecture
  set(NODES_FCT_DIR
      "${CMAKE_CURRENT_SOURCE_DIR}/src/model/nodes/nodes_function")

  file(GLOB ALL_NODE_SOURCES "${NODES_FCT_DIR}/*.cpp")
  set(MINIMAL_NODE_SET_SOURCES
      "${NODES_FCT_DIR}/broadcast.cpp"
      "${NODES_FCT_DIR}/debug.cpp"
      "${NODES_FCT_DIR}/flooding_uniform_level.cpp"
      "${NODES_FCT_DIR}/gabor_wave_fbm.cpp"
      "${NODES_FCT_DIR}/gain.cpp"
      "${NODES_FCT_DIR}/hydraulic_stream_log.cpp"
      "${NODES_FCT_DIR}/export_heightmap.cpp"
      "${NODES_FCT_DIR}/noise.cpp"
      "${NODES_FCT_DIR}/preview.cpp"
      "${NODES_FCT_DIR}/receive.cpp"
      "${NODES_FCT_DIR}/thru.cpp"
      "${NODES_FCT_DIR}/toggle.cpp")

  set(COMPLENTARY_NODE_SOURCES ${ALL_NODE_SOURCES})
  foreach(f ${MINIMAL_NODE_SET_SOURCES})
    list(REMOVE_ITEM COMPLENTARY_NODE_SOURCES ${f})
  endforeach()

  foreach(f ${COMPLENTARY_NODE_SOURCES})
    list(REMOVE_ITEM HESIOD_SOURCES ${f})
  endforeach()
endif()

# ------------------------------
# Executable
# ------------------------------
if(APPLE)
  add_executable(${PROJECT_NAME} MACOSX_BUNDLE)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "Hesiod"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.ottolink.hesiod"
    MACOSX_BUNDLE_BUNDLE_VERSION "${hesiod-root_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${hesiod-root_VERSION_MAJOR}.${hesiod-root_VERSION_MINOR}"
  )
else()
  add_executable(${PROJECT_NAME})
endif()

target_sources(${PROJECT_NAME} PRIVATE ${HESIOD_SOURCES} ${HESIOD_GUI_INCLUDES})

# ------------------------------
# Include directories
# ------------------------------
target_include_directories(
  ${PROJECT_NAME}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include
          ${OPENGL_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})

# ------------------------------
# Compiler features
# ------------------------------
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

# Hesiod 0.6 feature definitions
if(HESIOD_ENABLE_SMART_PREVIEW_CACHE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE HESIOD_HAS_SMART_PREVIEW_CACHE)
endif()
if(HESIOD_ENABLE_TERMINAL_LOGGING)
  target_compile_definitions(${PROJECT_NAME} PRIVATE HESIOD_HAS_TERMINAL_LOGGING)
endif()
if(HESIOD_ENABLE_NODE_EDITOR_POLISH)
  target_compile_definitions(${PROJECT_NAME} PRIVATE HESIOD_HAS_NODE_EDITOR_POLISH)
endif()

# ------------------------------
# Link libraries
# ------------------------------
target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE hesiod_options
          hesiod_platform
          hesiod_qt_logging
          args
          spdlog::spdlog
          nlohmann_json::nlohmann_json
          highmap
          gnode
          gnodegui
          attributes
          Qt6::Core
          Qt6::OpenGL
          Qt6::Widgets
          Qt6::OpenGLWidgets
          qterrain-renderer
          qtexture_downloader)

# ------------------------------
# Vulkan GPU compute (optional)
# ------------------------------
if(HESIOD_ENABLE_VULKAN)
  # Shader compilation (GLSL -> SPIR-V)
  set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
  set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
  file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

  set(VULKAN_SHADERS
    ${SHADER_DIR}/vector_add.comp
    ${SHADER_DIR}/noise_fbm.comp
    ${SHADER_DIR}/abs.comp
    ${SHADER_DIR}/inverse.comp
    ${SHADER_DIR}/shift_elevation.comp
    ${SHADER_DIR}/clamp.comp
    ${SHADER_DIR}/remap.comp
    ${SHADER_DIR}/rescale.comp
    ${SHADER_DIR}/fold.comp
    ${SHADER_DIR}/smoothstep.comp
    ${SHADER_DIR}/gamma_correction.comp
    ${SHADER_DIR}/gain.comp
    ${SHADER_DIR}/saturate.comp
    ${SHADER_DIR}/gaussian_decay.comp
    ${SHADER_DIR}/cos.comp
    ${SHADER_DIR}/combiner_add.comp
    ${SHADER_DIR}/combiner_subtract.comp
    ${SHADER_DIR}/combiner_multiply.comp
    ${SHADER_DIR}/combiner_divide.comp
    ${SHADER_DIR}/combiner_min.comp
    ${SHADER_DIR}/combiner_max.comp
    ${SHADER_DIR}/combiner_blend.comp
    ${SHADER_DIR}/normal_map.comp
    ${SHADER_DIR}/select_slope.comp
    ${SHADER_DIR}/hydraulic_blur.comp
    ${SHADER_DIR}/gabor_wave_fbm.comp
    ${SHADER_DIR}/hydraulic_erosion.comp)

  foreach(SHADER ${VULKAN_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    set(SPIRV_OUTPUT ${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv)
    add_custom_command(
      OUTPUT ${SPIRV_OUTPUT}
      COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${SHADER} -o ${SPIRV_OUTPUT}
      DEPENDS ${SHADER}
      COMMENT "Compiling Vulkan shader: ${SHADER_NAME}.comp")
    list(APPEND SPIRV_OUTPUTS ${SPIRV_OUTPUT})
  endforeach()

  add_custom_target(compile_shaders DEPENDS ${SPIRV_OUTPUTS})
  add_dependencies(${PROJECT_NAME} compile_shaders)

  # Generate shader paths header
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/hesiod/gpu/vulkan/shader_paths.hpp.in
    ${CMAKE_BINARY_DIR}/include/hesiod/gpu/vulkan/shader_paths.hpp @ONLY)

  target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
endif()

# ------------------------------
# Copy data folder
# ------------------------------
if(APPLE)
  # For macOS bundles, copy data into the Resources directory
  file(COPY ${CMAKE_SOURCE_DIR}/Hesiod/data
       DESTINATION ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.app/Contents/Resources/.)
  # Also copy to the plain output path for non-bundle / development use
  file(COPY ${CMAKE_SOURCE_DIR}/Hesiod/data
       DESTINATION ${EXECUTABLE_OUTPUT_PATH}/.)
else()
  file(COPY ${CMAKE_SOURCE_DIR}/Hesiod/data
       DESTINATION ${EXECUTABLE_OUTPUT_PATH}/.)
endif()

file(COPY ${CMAKE_SOURCE_DIR}/Hesiod/data
     DESTINATION ${EXECUTABLE_OUTPUT_PATH}/../.)

# ------------------------------
# Build information messages
# ------------------------------
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
