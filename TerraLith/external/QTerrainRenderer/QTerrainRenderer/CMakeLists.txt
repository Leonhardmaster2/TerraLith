project(qterrain-renderer)

# Qt6
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOMOC_VERBOSE ON)

# --- sources GUI headers need to be added in add_executable, otherwise the moc
# won't parse them
file(GLOB_RECURSE QTR_GUI_INCLUDES
     ${CMAKE_CURRENT_SOURCE_DIR}/include/qtr/*.hpp)

file(GLOB_RECURSE QTR_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

set(QTR_INCLUDE PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE)

add_library(${PROJECT_NAME} STATIC ${QTR_SOURCES} ${QTR_GUI_INCLUDES})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_include_directories(${PROJECT_NAME} PRIVATE ${QTR_INCLUDE}
                                                   ${CMAKE_BINARY_DIR}/include)

# data (two locations for convenience aka lazyness)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data
     DESTINATION ${EXECUTABLE_OUTPUT_PATH}/.)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data
     DESTINATION ${EXECUTABLE_OUTPUT_PATH}/../.)

# Link libraries
target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC spdlog::spdlog
         nlohmann_json::nlohmann_json
         Qt6::Core
         Qt6::Widgets
         Qt6::OpenGL
         Qt6::OpenGLWidgets
         imgui::imgui
         glfw)
