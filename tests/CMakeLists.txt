include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.14.0
)
FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

add_executable(EngineUnitTests
    unit/Core/Test_ConsoleCapture.cpp
    unit/Core/Test_Logger.cpp
    unit/Core/Test_Paths.cpp
)

target_link_libraries(EngineUnitTests
    PRIVATE
        Engine
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
    PROPERTIES LABELS "unit"
)
