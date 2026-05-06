# -------------------------
# Engine library
# -------------------------

# -------------------------
# Dependencies
# -------------------------

include(FetchContent)

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
    Engine/Runtime/Context.cpp
    Engine/Windowing/WindowSystem.cpp
)

# Make a library so external projects can link to it if needed.
# The main executable will also link to this.
add_library(Engine STATIC ${ENGINE_SOURCES})

target_include_directories(Engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link spdlog to Engine (IMPORTANT)
target_link_libraries(Engine PUBLIC spdlog::spdlog)

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

find_package(SDL3 REQUIRED CONFIG)

# create executable
add_executable(GameEngine main.cpp)

# link main executable to Engine library
target_link_libraries(GameEngine PRIVATE Engine SDL3::SDL3)
