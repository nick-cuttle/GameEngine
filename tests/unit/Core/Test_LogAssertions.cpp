#include <LogAssertions.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("LogAssertions", "[unit][core][logger]")
{
    SECTION("matches file log messages without date time information")
    {
        Engine::Tests::confirmLogMessage(
            "[2026-05-06 18:05:07.409] [error] [Engine] Test Error Message!\n",
            Engine::Tests::kErrorLogType, Engine::Tests::kEngineLoggerName, "Test Error Message!");
    }

    SECTION("matches console log messages without time information")
    {
        Engine::Tests::confirmLogMessage("[18:05:07] [error] [Engine] Test Error Message!\n",
                                         Engine::Tests::kErrorLogType,
                                         Engine::Tests::kEngineLoggerName, "Test Error Message!");
    }

    SECTION("finds expected messages inside multiple log lines")
    {
        auto const output =
            "[18:05:07] [info] [Engine] ignored\n"
            "[18:05:08] [warning] [Renderer] Device recreated\n";

        Engine::Tests::confirmLogMessage(output, "warning", "Renderer", "Device recreated");
    }

    SECTION("matches each known log type with convenience helpers")
    {
        auto const output =
            "[18:05:07] [trace] [Engine] trace message\n"
            "[18:05:08] [debug] [Engine] debug message\n"
            "[18:05:09] [info] [Engine] info message\n"
            "[18:05:10] [warning] [Engine] warning message\n"
            "[18:05:11] [error] [Engine] error message\n"
            "[18:05:12] [critical] [Engine] critical message\n";

        Engine::Tests::confirmTraceLogMessage(output, Engine::Tests::kEngineLoggerName,
                                              "trace message");
        Engine::Tests::confirmDebugLogMessage(output, Engine::Tests::kEngineLoggerName,
                                              "debug message");
        Engine::Tests::confirmInfoLogMessage(output, Engine::Tests::kEngineLoggerName,
                                             "info message");
        Engine::Tests::confirmWarningLogMessage(output, Engine::Tests::kEngineLoggerName,
                                                "warning message");
        Engine::Tests::confirmErrorLogMessage(output, Engine::Tests::kEngineLoggerName,
                                              "error message");
        Engine::Tests::confirmCriticalLogMessage(output, Engine::Tests::kEngineLoggerName,
                                                 "critical message");
    }
}
