# -------------------------
# Engine library
# -------------------------

include(${CMAKE_SOURCE_DIR}/cmake/Dependencies.cmake)
engine_find_or_fetch_sdl3()
engine_find_or_fetch_fmt()
engine_find_or_fetch_spdlog()
engine_find_or_fetch_glad_gl_core()

set(ENGINE_SOURCES
    Engine/Core/Paths.cpp
    Engine/Core/Logger.cpp
    Engine/Input/InputSystem.cpp
    Engine/Rendering/Internal/OpenGLRenderingBackend.cpp
    Engine/Rendering/Renderer.cpp
    Engine/Runtime/Context.cpp
    Engine/Windowing/WindowSystem.cpp
)

add_library(Engine STATIC ${ENGINE_SOURCES})

target_include_directories(Engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# fmt is part of the public Logger formatting API; implementation dependencies stay private.
target_link_libraries(Engine
    PUBLIC
        fmt::fmt
    PRIVATE
        glad_gl_core_46
        OpenGL::GL
        SDL3::SDL3
        spdlog::spdlog
)

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

add_executable(GameEngine main.cpp)

target_link_libraries(GameEngine PRIVATE Engine)

install(
    TARGETS Engine GameEngine
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Engine
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.hpp"
)

install(
    FILES
        ${CMAKE_SOURCE_DIR}/README.md
        ${CMAKE_SOURCE_DIR}/CONTEXT.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/docs/adr
    DESTINATION ${CMAKE_INSTALL_DOCDIR}/docs
    FILES_MATCHING
        PATTERN "*.md"
)
