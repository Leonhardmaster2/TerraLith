project(qtexture_downloader)

# Qt6
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOMOC_VERBOSE ON)

# --- sources GUI headers need to be added in add_executable, otherwise the moc
# won't parse them
file(GLOB_RECURSE QTD_GUI_INCLUDES
     ${CMAKE_CURRENT_SOURCE_DIR}/include/qtd/*.hpp)

file(GLOB_RECURSE QTD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

set(QTD_INCLUDE PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE)

add_library(${PROJECT_NAME} STATIC ${QTD_SOURCES} ${QTD_GUI_INCLUDES})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_include_directories(${PROJECT_NAME} PRIVATE ${QTD_INCLUDE}
                                                   ${CMAKE_BINARY_DIR}/include)

# Link libraries
target_link_libraries(
  ${PROJECT_NAME} PUBLIC spdlog::spdlog nlohmann_json::nlohmann_json Qt6::Core
                         Qt6::Widgets Qt6::Network)
