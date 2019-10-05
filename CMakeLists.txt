if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif ()

project(template)
cmake_minimum_required(VERSION 3.11)
include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-long-long -pedantic -fPIC")
set(CMAKE_ENABLE_COMPILE_COMMANDS "ON")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
elseif (${CMAKE_BUILD_TYPE} STREQUAL "Release")
  message("-- Release mode, all optimizations enabled")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Ofast -march=native")
endif ()

# Google Test
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.8.1
)
FetchContent_GetProperties(googletest)
if (NOT googletest_POPULATED)
  FetchContent_Populate(googletest)
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif ()
include(GoogleTest)

# Google Log
FetchContent_Declare(
        glog
        GIT_REPOSITORY https://github.com/google/glog.git
        GIT_TAG v0.4.0
)
FetchContent_GetProperties(glog)
if (NOT glog_POPULATED)
  FetchContent_Populate(glog)
  set(WITH_GFLAGS OFF CACHE BOOL "we don't want gflags")
  add_subdirectory(${glog_SOURCE_DIR} ${glog_BINARY_DIR})
endif()


enable_testing()

add_subdirectory(tests)

