cmake_minimum_required(VERSION 3.16)

project(GameEngine VERSION 0.1.0 LANGUAGES C CXX)

include(GNUInstallDirs)

option(ENGINE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(BUILD_TESTS "Build engine tests" ON)
# Prefer installed packages, but keep source fetching available for clone-and-build setups.
option(ENGINE_FETCH_DEPENDENCIES "Fetch missing third-party dependencies from source" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src)

include(CTest)

if(BUILD_TESTING AND BUILD_TESTS)
    add_subdirectory(tests)
endif()
