cmake_minimum_required(VERSION 3.10)

project(GameEngine LANGUAGES CXX)

option(ENGINE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# include CMakeLists.txt files from src
add_subdirectory(src)
