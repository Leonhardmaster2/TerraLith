include(dkm.cmake)

set(NANOFLANN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(NANOFLANN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(nanoflann)