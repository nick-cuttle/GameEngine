/**
 * @file  Test_Paths.cpp
 * @brief Tests the Paths class for managing engine paths.
 */

#include <Engine/Core/Paths.hpp>
#include <TestTempDirectory.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Paths", "[unit][core][paths]")
{
    Engine::Tests::TestTempDirectory tempDirectory("paths-base-override");

    Engine::Paths paths;
    paths.initialize({.baseOverride = tempDirectory.path()});

    SECTION("initialize")
    {
        // confirm paths are correct.
        REQUIRE(paths.base() == tempDirectory.path());
        REQUIRE(paths.logs() == tempDirectory.path() / "logs");
        REQUIRE(paths.assets() == tempDirectory.path() / "assets");

        // confirm log directory is created during initialization.
        REQUIRE(std::filesystem::is_directory(paths.logs()));
    }
}
