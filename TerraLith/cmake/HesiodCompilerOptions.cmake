add_library(hesiod_options INTERFACE)

# Require C++20
target_compile_features(hesiod_options INTERFACE cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  message(STATUS "GNU|Clang compiler")

  # Common flags (shared between GCC and Clang/Apple Clang)
  target_compile_options(
    hesiod_options
    INTERFACE -Ofast -ffast-math -Wall -Wextra
              -Wno-deprecated-declarations)

  # GCC-only warning flags (not supported by Apple Clang)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(
      hesiod_options
      INTERFACE -Wno-dangling-reference)
  endif()

  # Debug flags
  target_compile_options(
    hesiod_options
    INTERFACE $<$<CONFIG:Debug>:
              -g
              -DLOG_LEVEL=3
              -fno-omit-frame-pointer
              -Wpedantic
              -Wno-unused-parameter
              -Wcast-align
              -Wcast-qual
              -Wnull-dereference
              -Wnon-virtual-dtor
              -Woverloaded-virtual
              -Wzero-as-null-pointer-constant
              $<$<BOOL:${HESIOD_UNUSED_FUNCTIONS}>:-Wunused-function>
              >)

  # GCC-only debug warning flags
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(
      hesiod_options
      INTERFACE $<$<CONFIG:Debug>:
                -Wno-psabi
                -Wduplicated-cond
                -Wduplicated-branches
                -Wlogical-op
                -Wuseless-cast
                >)
  endif()

  # Release flags
  target_compile_options(hesiod_options
                         INTERFACE $<$<CONFIG:Release>:-DLOG_LEVEL=3>)

  # LTO and function sections (optional)
  if(HESIOD_ENABLE_LTO)
    message(STATUS "HESIOD_ENABLE_LTO is enabled")
    target_compile_options(hesiod_options INTERFACE -ffunction-sections
                                                    -fdata-sections -flto)

    if(APPLE)
      # macOS uses -dead_strip instead of --gc-sections
      target_link_options(hesiod_options INTERFACE -Wl,-dead_strip)
    else()
      target_link_options(hesiod_options INTERFACE -Wl,--gc-sections
                          -Wl,--print-gc-sections)
    endif()
  endif()

  if(HESIOD_MINIMAL_NODE_SET)
    message(STATUS "HESIOD_MINIMAL_NODE_SET is enabled")
    target_compile_definitions(hesiod_options INTERFACE HESIOD_MINIMAL_NODE_SET)
  endif()
endif()

# MSVC options
if(MSVC)
  message(STATUS "MSVC compiler")
  target_compile_options(hesiod_options INTERFACE /W4)
  add_compile_definitions(M_PI=3.14159265358979323846)
endif()
