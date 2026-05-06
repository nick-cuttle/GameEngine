#include <Engine/Core/Paths.hpp>
#include <TestTempDirectory.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("PathsTests", "[unit][core][paths]")
{
    Engine::Tests::TestTempDirectory tempDirectory("paths-base-override");

    Engine::Paths paths;
    paths.initialize({.baseOverride = tempDirectory.path()});

    SECTION("base path uses the configured override")
    {
        REQUIRE(paths.base() == tempDirectory.path());
    }

    SECTION("engine directories are derived from the base path")
    {
        REQUIRE(paths.logs() == tempDirectory.path() / "logs");
        REQUIRE(paths.assets() == tempDirectory.path() / "assets");
    }

    SECTION("logs directory is created during initialization")
    {
        REQUIRE(std::filesystem::is_directory(paths.logs()));
    }
}
