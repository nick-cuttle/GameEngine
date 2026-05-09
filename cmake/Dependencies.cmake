include(FetchContent)

# Keep dependency lookup package-manager friendly by trying find_package first, then falling back
# to FetchContent only when the dependency is missing and fallback fetching is enabled.

function(engine_configure_sdl_fetch_options)
    # The engine currently needs SDL video/windowing, not SDL GPU or Vulkan support.
    if(NOT ENGINE_ENABLE_SDL_GPU)
        set(SDL_GPU OFF CACHE BOOL "Enable SDL GPU support" FORCE)
        set(SDL_RENDER_GPU OFF CACHE BOOL "Enable the SDL GPU render driver" FORCE)
    endif()

    if(NOT ENGINE_ENABLE_SDL_VULKAN)
        set(SDL_VULKAN OFF CACHE BOOL "Enable Vulkan support in SDL" FORCE)
        set(SDL_RENDER_VULKAN OFF CACHE BOOL "Enable the SDL Vulkan render driver" FORCE)
    endif()
endfunction()

function(engine_require_dependency dependency_name dependency_target)
    # Fail with the same guidance regardless of whether lookup or fallback fetching was attempted.
    if(NOT TARGET ${dependency_target})
        message(FATAL_ERROR
            "${dependency_name} was not found. Install it with your package manager, "
            "configure with a package-manager toolchain such as vcpkg, or set "
            "ENGINE_FETCH_DEPENDENCIES=ON to fetch missing dependencies from source."
        )
    endif()
endfunction()

function(engine_find_or_fetch_sdl3)
    option(SDL_FORCE_FETCH "Always fetch SDL from source instead of using find_package" OFF)
    option(ENGINE_ENABLE_SDL_GPU "Enable SDL GPU support when building fetched SDL" OFF)
    option(ENGINE_ENABLE_SDL_VULKAN "Enable SDL Vulkan support when building fetched SDL" OFF)

    # SDL is the heaviest dependency, so prefer an installed package unless explicitly overridden.
    if(NOT SDL_FORCE_FETCH)
        find_package(SDL3 CONFIG QUIET)
    endif()

    if(NOT TARGET SDL3::SDL3 AND ENGINE_FETCH_DEPENDENCIES)
        engine_configure_sdl_fetch_options()

        FetchContent_Declare(
            SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-3.4.8
        )
        FetchContent_MakeAvailable(SDL3)
    endif()

    engine_require_dependency("SDL3" "SDL3::SDL3")
endfunction()

function(engine_find_or_fetch_fmt)
    find_package(fmt CONFIG QUIET)

    if(NOT TARGET fmt::fmt AND ENGINE_FETCH_DEPENDENCIES)
        FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG 12.1.0
        )
        FetchContent_MakeAvailable(fmt)
    endif()

    engine_require_dependency("fmt" "fmt::fmt")
endfunction()

function(engine_find_or_fetch_spdlog)
    find_package(spdlog CONFIG QUIET)

    if(NOT TARGET spdlog::spdlog AND ENGINE_FETCH_DEPENDENCIES)
        # Reuse the engine's fmt dependency instead of letting fetched spdlog bring its own copy.
        set(SPDLOG_FORCE_FETCH ON CACHE BOOL "Force fetching bundled spdlog dependencies" FORCE)
        set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use external fmt library" FORCE)

        FetchContent_Declare(
            spdlog
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG v1.17.0
        )
        FetchContent_MakeAvailable(spdlog)
    endif()

    engine_require_dependency("spdlog" "spdlog::spdlog")
endfunction()

function(engine_find_or_fetch_glad_gl_core)
    if(NOT TARGET glad_gl_core_46 AND ENGINE_FETCH_DEPENDENCIES)
        FetchContent_Declare(
            glad
            GIT_REPOSITORY https://github.com/Dav1dde/glad.git
            GIT_TAG v2.0.8
            SOURCE_SUBDIR cmake
        )
        FetchContent_MakeAvailable(glad)
        glad_add_library(glad_gl_core_46 STATIC REPRODUCIBLE API gl:core=4.6)
    endif()

    engine_require_dependency("glad" "glad_gl_core_46")
    find_package(OpenGL REQUIRED)
endfunction()

function(engine_find_or_fetch_catch2)
    find_package(Catch2 3 CONFIG QUIET)

    if(NOT TARGET Catch2::Catch2WithMain AND ENGINE_FETCH_DEPENDENCIES)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.14.0
        )
        FetchContent_MakeAvailable(Catch2)
        list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    elseif(Catch2_DIR)
        # Packaged Catch2 exports Catch.cmake beside Catch2Config.cmake; include(Catch) needs this.
        list(APPEND CMAKE_MODULE_PATH ${Catch2_DIR})
    endif()

    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)
    engine_require_dependency("Catch2" "Catch2::Catch2WithMain")
endfunction()
