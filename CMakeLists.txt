cmake_minimum_required(VERSION 3.16)

project(GameEngine LANGUAGES CXX)

option(ENGINE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(SDL_FORCE_FETCH "Fetch SDL from source" ON)
option(ENGINE_ENABLE_SDL_GPU "Enable SDL GPU support when building fetched SDL" OFF)
option(ENGINE_ENABLE_SDL_VULKAN "Enable SDL Vulkan support when building fetched SDL" OFF)
option(BUILD_TESTS "Build engine tests" ON)
option(SDL_FORCE_FETCH "Fetch SDL from source" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

if(SDL_FORCE_FETCH)
    if(NOT ENGINE_ENABLE_SDL_GPU)
        set(SDL_GPU OFF CACHE BOOL "Enable SDL GPU support" FORCE)
        set(SDL_RENDER_GPU OFF CACHE BOOL "Enable the SDL GPU render driver" FORCE)
    endif()

    if(NOT ENGINE_ENABLE_SDL_VULKAN)
        set(SDL_VULKAN OFF CACHE BOOL "Enable Vulkan support in SDL" FORCE)
        set(SDL_RENDER_VULKAN OFF CACHE BOOL "Enable the SDL Vulkan render driver" FORCE)
    endif()

    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.8
    )
    FetchContent_MakeAvailable(SDL3)
else()
    find_package(SDL3 REQUIRED CONFIG)
endif()

# include CMakeLists.txt files from src
add_subdirectory(src)

include(CTest)

if(BUILD_TESTING AND BUILD_TESTS)
    add_subdirectory(tests)
endif()
