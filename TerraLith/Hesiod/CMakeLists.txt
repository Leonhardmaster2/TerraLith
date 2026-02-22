project(hesiod LANGUAGES CXX)

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
