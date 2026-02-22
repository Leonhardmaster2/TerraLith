cmake_minimum_required(VERSION 3.22)

project(clerrorlookup-root)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# --- library

add_subdirectory(${PROJECT_SOURCE_DIR}/CLErrorLookup)
add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
