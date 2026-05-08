/**
 * @file  Test_LoggingSystem.cpp
 * @brief Tests the engine-owned logging system and logger handles.
 */

#include <Engine/Core/Logger.hpp>
#include <LogAssertions.hpp>
#include <SpdlogTestGuard.hpp>
#include <TestTempDirectory.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace
{
constexpr auto kEngineLoggerName = "Engine";
constexpr auto kWindowSystemLoggerName = "WindowSystem";

/// @brief Reads the entire content of a text file.
/// @param[in] path The path to the file to read.
/// @return The content of the file as a string.
std::string readTextFile(std::filesystem::path const &path)
{
    std::ifstream file(path);
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

} // namespace

TEST_CASE("LoggingSystem", "[unit][core][logging-system]")
{
    Engine::Tests::TestTempDirectory tempDirectory("Test_LoggingSystem");
    Engine::Tests::SpdlogTestGuard spdlog;

    Engine::LoggingSystem loggingSystem;

    std::filesystem::path const logDir = tempDirectory.path() / "logs";
    std::filesystem::path const kLogFile = tempDirectory.path() / "logs" / "Engine.log";

    SECTION("root before initialize is rejected")
    {
        REQUIRE_THROWS(loggingSystem.root());
    }

    SECTION("createSubsystemLogger before initialize is rejected")
    {
        REQUIRE_THROWS(loggingSystem.createSubsystemLogger(kWindowSystemLoggerName));
    }

    loggingSystem.initialize({.logDirectory = logDir});
    loggingSystem.flush();

    SECTION("initialize")
    {
        REQUIRE(std::filesystem::is_directory(logDir));
        REQUIRE(std::filesystem::exists(kLogFile));

        std::string const logOutput = readTextFile(kLogFile);

        // confirm expected initialization log messages are present in console output
        Engine::Tests::confirmInfoLogMessage(logOutput, kEngineLoggerName,
                                             "Engine logger initialized");
        Engine::Tests::confirmInfoLogMessage(logOutput, kEngineLoggerName,
                                             "Log file: " + kLogFile.string());
    }

    SECTION("initialize twice is rejected")
    {
        REQUIRE_THROWS(loggingSystem.initialize({.logDirectory = logDir}));
    }

    SECTION("root logger writes under Engine")
    {
        loggingSystem.root().error("Test Error Message!");
        loggingSystem.flush();

        Engine::Tests::confirmLogMessage(readTextFile(kLogFile), "error", kEngineLoggerName,
                                         "Test Error Message!");
    }

    SECTION("subsystem logger writes under subsystem name")
    {
        Engine::Logger subsystemLogger =
            loggingSystem.createSubsystemLogger(kWindowSystemLoggerName);
        subsystemLogger.info("{} logging is active", kWindowSystemLoggerName);
        subsystemLogger.flush();

        Engine::Tests::confirmInfoLogMessage(readTextFile(kLogFile), kWindowSystemLoggerName,
                                             "WindowSystem logging is active");
    }

    SECTION("duplicate subsystem logger names are rejected")
    {
        loggingSystem.createSubsystemLogger(kWindowSystemLoggerName);

        REQUIRE_THROWS(loggingSystem.createSubsystemLogger(kWindowSystemLoggerName));
    }

    SECTION("flush flushes logger handles")
    {
        loggingSystem.root().warn("Flush writes this warning");
        loggingSystem.flush();

        Engine::Tests::confirmWarningLogMessage(readTextFile(kLogFile), kEngineLoggerName,
                                                "Flush writes this warning");
    }

    SECTION("issued logger handles reject writes after shutdown")
    {
        Engine::Logger rootLogger = loggingSystem.root();

        Engine::Logger subsystemLogger =
            loggingSystem.createSubsystemLogger(kWindowSystemLoggerName);

        loggingSystem.shutdown();

        try
        {
            rootLogger.error("Write after shutdown");
            FAIL("Expected logger writes after shutdown to throw");
        }
        catch (std::runtime_error const &error)
        {
            REQUIRE(std::string(error.what()) ==
                    "Logger 'Engine' cannot write after LoggingSystem shutdown");
        }

        try
        {
            subsystemLogger.error("Write after shutdown");
            FAIL("Expected logger writes after shutdown to throw");
        }
        catch (std::runtime_error const &error)
        {
            REQUIRE(std::string(error.what()) ==
                    "Logger 'WindowSystem' cannot write after LoggingSystem shutdown");
        }
    }

    SECTION("critical logger writes under Engine")
    {
        loggingSystem.root().critical("Critical failure {}", 42);
        loggingSystem.flush();

        Engine::Tests::confirmCriticalLogMessage(readTextFile(kLogFile), kEngineLoggerName,
                                                 "Critical failure 42");
    }

    SECTION("debug logger writes under Engine in debug builds")
    {
        loggingSystem.root().debug("Debug value {}", 7);
        loggingSystem.flush();

        std::string const logOutput = readTextFile(kLogFile);

#ifdef NDEBUG
        REQUIRE_FALSE(Engine::Tests::containsLogMessage(logOutput, Engine::Tests::kDebugLogType,
                                                        kEngineLoggerName, "Debug value 7"));
#else
        Engine::Tests::confirmDebugLogMessage(logOutput, kEngineLoggerName, "Debug value 7");
#endif
    }

    loggingSystem.shutdown();
}
