# -------------------------
# Engine library
# -------------------------

# -------------------------
# Dependencies
# -------------------------

include(FetchContent)

option(SDL_FORCE_FETCH "Fetch SDL from source" ON)
option(ENGINE_ENABLE_SDL_GPU "Enable SDL GPU support when building fetched SDL" OFF)
option(ENGINE_ENABLE_SDL_VULKAN "Enable SDL Vulkan support when building fetched SDL" OFF)

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

# Fetch fmt for the public Logger formatting API.
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 12.1.0
)
FetchContent_MakeAvailable(fmt)

# Fetch spdlog
set(SPDLOG_FORCE_FETCH ON)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(spdlog)


set(ENGINE_SOURCES
    Engine/Core/Paths.cpp
    Engine/Core/Logger.cpp
    Engine/Input/InputSystem.cpp
    Engine/Runtime/Context.cpp
    Engine/Windowing/WindowSystem.cpp
)

# Make a library so external projects can link to it if needed.
# The main executable will also link to this.
add_library(Engine STATIC ${ENGINE_SOURCES})

target_include_directories(Engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# fmt is part of the public Logger formatting API; spdlog remains hidden in Logger.cpp.
target_link_libraries(Engine PUBLIC fmt::fmt PRIVATE spdlog::spdlog)

# Set compiler warnings for the Engine library.
if(MSVC)
    target_compile_options(Engine PRIVATE /W4)
    if(ENGINE_WARNINGS_AS_ERRORS)
        target_compile_options(Engine PRIVATE /WX)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Engine PRIVATE -Wall -Wextra -Wpedantic)
    if(ENGINE_WARNINGS_AS_ERRORS)
        target_compile_options(Engine PRIVATE -Werror)
    endif()
endif()
target_link_libraries(Engine PRIVATE SDL3::SDL3)

# create executable
add_executable(GameEngine main.cpp)

# link main executable to Engine library
target_link_libraries(GameEngine PRIVATE Engine)
