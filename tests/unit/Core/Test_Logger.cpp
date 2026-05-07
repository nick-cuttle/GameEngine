/**
 * @file  Test_Logger.cpp
 * @brief Tests the Logger class, which manages logging functionality for the engine.
 */

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

    std::filesystem::path const logDir = tempDirectory.path() / "logs";
    std::filesystem::path const kLogFile = tempDirectory.path() / "logs" / "Engine.log";

    logger.initialize({.logDirectory = logDir});
    logger.root()->flush();

    SECTION("initialize")
    {
        REQUIRE(logger.root() != nullptr);
        REQUIRE(logger.root()->name() == kEngineLoggerName);

        REQUIRE(std::filesystem::is_directory(logDir));
        REQUIRE(std::filesystem::exists(kLogFile));

        std::string const logOutput = readTextFile(kLogFile);

        // confirm expected initialization log messages are present in console output
        Engine::Tests::confirmInfoLogMessage(logOutput, kEngineLoggerName,
                                             "Engine logger initialized");
        Engine::Tests::confirmInfoLogMessage(logOutput, kEngineLoggerName,
                                             "Log file: " + kLogFile.string());
    }

    SECTION("file log messages can be confirmed without date time information")
    {
        auto const logFile = tempDirectory.path() / "logs" / "Engine.log";

        logger.root()->error("Test Error Message!");
        logger.root()->flush();

        Engine::Tests::confirmLogMessage(readTextFile(logFile), "error", kEngineLoggerName,
                                         "Test Error Message!");
    }

    SECTION("createSubsystem")
    {
        auto subsystem = logger.createSubsystem("LoggerTests.Subsystem");

        REQUIRE(subsystem != nullptr);
        REQUIRE(subsystem->name() == "LoggerTests.Subsystem");
        REQUIRE(subsystem->level() == logger.root()->level());
    }

    SECTION("duplicate subsystem logger names are rejected")
    {
        constexpr auto kDuplicateName = "LoggerTests.DuplicateSubsystem";
        logger.createSubsystem(kDuplicateName);

        REQUIRE_THROWS(logger.createSubsystem(kDuplicateName));
    }
}
