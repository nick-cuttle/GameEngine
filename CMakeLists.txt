cmake_minimum_required(VERSION 3.10)
project(GameEngine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Variables
set(BIN_DIR ${CMAKE_BINARY_DIR}/bin)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(LIB_DIR ${CMAKE_SOURCE_DIR}/lib)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})

set(SOURCE_FILES ${SRC_DIR}/main.cpp)

add_executable(GameEngine ${SOURCE_FILES})

target_compile_options(GameEngine PRIVATE
    -Wall
    -Werror
)

