#include <Engine/Core/Logger.hpp>
#include <LogAssertions.hpp>
#include <SpdlogTestGuard.hpp>
#include <TestTempDirectory.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace
{
// constants
constexpr auto kEngineLoggerName = "Engine";

std::string readTextFile(std::filesystem::path const &path)
{
    std::ifstream file(path);
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

} // namespace

TEST_CASE("Logger", "[unit][core][logger]")
{
    Engine::Tests::TestTempDirectory tempDirectory("Test_Logger");
    Engine::Tests::SpdlogTestGuard spdlog;

    Engine::Logger logger;
    logger.initialize({.logDirectory = tempDirectory.path() / "logs"});

    SECTION("root logger is initialized")
    {
        REQUIRE(logger.root() != nullptr);
        REQUIRE(logger.root()->name() == kEngineLoggerName);
    }

    SECTION("log directory and file are created")
    {
        auto const logFile = tempDirectory.path() / "logs" / "Engine.log";

        REQUIRE(std::filesystem::is_directory(tempDirectory.path() / "logs"));
        REQUIRE(std::filesystem::exists(logFile));
    }

    SECTION("file log messages can be confirmed without date time information")
    {
        auto const logFile = tempDirectory.path() / "logs" / "Engine.log";

        logger.root()->error("Test Error Message!");
        logger.root()->flush();

        Engine::Tests::confirmLogMessage(readTextFile(logFile), "error", kEngineLoggerName,
                                         "Test Error Message!");
    }

    SECTION("subsystem loggers inherit the root logger level")
    {
        auto subsystem = logger.createSubsystem("LoggerTests.Subsystem");

        REQUIRE(subsystem != nullptr);
        REQUIRE(subsystem->name() == "LoggerTests.Subsystem");
        REQUIRE(subsystem->level() == logger.root()->level());
    }

    SECTION("duplicate subsystem logger names are rejected")
    {
        constexpr auto kSubsystemName = "LoggerTests.DuplicateSubsystem";
        logger.createSubsystem(kSubsystemName);

        REQUIRE_THROWS(logger.createSubsystem(kSubsystemName));
    }
}
