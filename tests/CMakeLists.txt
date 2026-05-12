include(${CMAKE_SOURCE_DIR}/cmake/Dependencies.cmake)
engine_find_or_fetch_sdl3()
engine_find_or_fetch_spdlog()
engine_find_or_fetch_catch2()
include(Catch)

add_executable(EngineUnitTests
    unit/Core/Test_ConsoleCapture.cpp
    unit/Core/Test_LogAssertions.cpp
    unit/Core/Test_LoggingSystem.cpp
    unit/Core/Test_Paths.cpp
    unit/Input/Test_InputSystem.cpp
    unit/Runtime/Test_Context.cpp
    unit/Windowing/Test_WindowSystem.cpp
)

target_link_libraries(EngineUnitTests
    PRIVATE
        Engine
        SDL3::SDL3
        spdlog::spdlog
        Catch2::Catch2WithMain
)

target_include_directories(EngineUnitTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_compile_definitions(EngineUnitTests
    PRIVATE
        GAMEENGINE_TEST_REPO_ROOT="${CMAKE_SOURCE_DIR}"
)

catch_discover_tests(EngineUnitTests
    # On Windows/vcpkg, app-local DLLs are copied after linking; defer discovery until CTest runs.
    DISCOVERY_MODE PRE_TEST
    PROPERTIES LABELS "unit"
)
