# -------------------------
# Engine library
# -------------------------

include(${CMAKE_SOURCE_DIR}/cmake/Dependencies.cmake)
engine_find_or_fetch_sdl3()
engine_find_or_fetch_fmt()
engine_find_or_fetch_spdlog()

set(ENGINE_SOURCES
    Engine/Core/Paths.cpp
    Engine/Core/Logger.cpp
    Engine/Input/InputSystem.cpp
    Engine/Runtime/Context.cpp
    Engine/Windowing/WindowSystem.cpp
)

add_library(Engine STATIC ${ENGINE_SOURCES})

target_include_directories(Engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# fmt is part of the public Logger formatting API; spdlog remains hidden in Logger.cpp.
target_link_libraries(Engine PUBLIC fmt::fmt PRIVATE spdlog::spdlog)

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

add_executable(GameEngine main.cpp)

target_link_libraries(GameEngine PRIVATE Engine)
