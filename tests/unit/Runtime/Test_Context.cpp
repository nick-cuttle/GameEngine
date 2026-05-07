/**
 * @file  Test_Context.cpp
 * @brief Tests the Context class, which manages engine-wide paths and logging.
 */

#include <Engine/Runtime/Context.hpp>
#include <SpdlogTestGuard.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>

TEST_CASE("Context", "[unit][runtime][context]")
{
    Engine::Tests::SpdlogTestGuard spdlog;

    Engine::Context context;
    context.initialize();
    context.logger.root()->flush();

    SECTION("initialize")
    {
        // check that paths are initialized with expected defaults and log directory is created.
        REQUIRE(!context.paths.base().empty());
        REQUIRE(context.paths.logs() == context.paths.base() / "logs");
        REQUIRE(context.paths.assets() == context.paths.base() / "assets");

        REQUIRE(std::filesystem::is_directory(context.paths.logs()));

        // confirm logger is initialized with expected defaults and log file is created.
        REQUIRE(context.logger.root() != nullptr);
        REQUIRE(context.logger.root()->name() == "Engine");
        REQUIRE(std::filesystem::exists(context.paths.logs() / "Engine.log"));
    }
}
