#include <Engine/Core/Logger.hpp>
#include <TestTempDirectory.hpp>

#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>

namespace
{
class SpdlogCleanup
{
  public:
    ~SpdlogCleanup()
    {
        spdlog::drop_all();
        spdlog::set_default_logger(nullptr);
    }
};

// constants
constexpr auto kEngineLoggerName = "Engine";

} // namespace

TEST_CASE("Logger", "[unit][core][logger]")
{
    SpdlogCleanup cleanup;
    Engine::Tests::TestTempDirectory tempDirectory("logger");

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
