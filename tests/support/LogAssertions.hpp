/**
 * @file  LogAssertions.hpp
 * @brief Defines test assertions for log output whose timestamp prefix changes per run.
 */

#pragma once

#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>
#include <string_view>

namespace Engine::Tests
{

/// @brief Canonical name of the root engine logger used by logger tests.
inline constexpr auto kEngineLoggerName = "Engine";
/// @brief Canonical spdlog level name for informational log messages.
inline constexpr auto kInfoLogType = "info";
/// @brief Canonical spdlog level name for warning log messages.
inline constexpr auto kWarningLogType = "warning";
/// @brief Canonical spdlog level name for error log messages.
inline constexpr auto kErrorLogType = "error";
/// @brief Canonical spdlog level name for debug log messages.
inline constexpr auto kDebugLogType = "debug";
/// @brief Canonical spdlog level name for critical log messages.
inline constexpr auto kCriticalLogType = "critical";
/// @brief Canonical spdlog level name for trace log messages.
inline constexpr auto kTraceLogType = "trace";

/// @brief Removes the first leading bracketed group from a log line.
/// @details Engine log patterns put timestamp information in the first bracket group.
/// @param[in] line The log line to normalize.
/// @return A view of the line after the first bracket group, or the original line if none exists.
inline std::string_view stripLeadingBracketGroup(std::string_view line)
{
    if (line.size() < 11 || line.front() != '[')
        return line;

    auto const endBracket = line.find(']');
    if (endBracket == std::string_view::npos)
        return line;

    auto const nextCharacter = endBracket + 1;
    if (nextCharacter < line.size() && line[nextCharacter] == ' ')
        return line.substr(nextCharacter + 1);

    return line.substr(nextCharacter);
}

/// @brief Builds the stable portion of an expected engine log message.
/// @param[in] errorType The log level text, such as "error" or "warning".
/// @param[in] loggerName The logger name expected in the message.
/// @param[in] message The message payload expected after the logger name.
/// @return The expected log text without timestamp information.
inline std::string buildExpectedLogMessage(std::string_view errorType, std::string_view loggerName,
                                           std::string_view message)
{
    std::string expected;
    expected.reserve(errorType.size() + loggerName.size() + message.size() + 7);
    expected.append("[");
    expected.append(errorType);
    expected.append("] [");
    expected.append(loggerName);
    expected.append("] ");
    expected.append(message);
    return expected;
}

/// @brief Checks whether a log buffer contains an expected log message.
/// @details Each line is normalized by removing the first bracket group before comparison.
/// @param[in] logOutput The captured console output or log file contents to search.
/// @param[in] errorType The log level text, such as "error" or "warning".
/// @param[in] loggerName The logger name expected in the message.
/// @param[in] message The message payload expected after the logger name.
/// @return True when a normalized line matches the expected message; otherwise false.
inline bool containsLogMessage(std::string_view logOutput, std::string_view errorType,
                               std::string_view loggerName, std::string_view message)
{
    auto const expected = buildExpectedLogMessage(errorType, loggerName, message);

    std::istringstream stream{std::string(logOutput)};
    std::string line;
    while (std::getline(stream, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (stripLeadingBracketGroup(line) == expected)
            return true;
    }

    return false;
}

/// @brief Requires that a log buffer contains an expected log message.
/// @param[in] logOutput The captured console output or log file contents to search.
/// @param[in] errorType The log level text, such as "error" or "warning".
/// @param[in] loggerName The logger name expected in the message.
/// @param[in] message The message payload expected after the logger name.
inline void confirmLogMessage(std::string_view logOutput, std::string_view errorType,
                              std::string_view loggerName, std::string_view message)
{
    INFO("Expected log message: " << buildExpectedLogMessage(errorType, loggerName, message));
    INFO("Actual log output: " << logOutput);
    REQUIRE(containsLogMessage(logOutput, errorType, loggerName, message));
}

/// @brief Requires that a log buffer contains an informational message.
/// @param[in] logOutput The captured console output or log file contents to search.
/// @param[in] loggerName The logger name expected in the message.
/// @param[in] message The message payload expected after the logger name.
inline void confirmInfoLogMessage(std::string_view logOutput, std::string_view loggerName,
                                  std::string_view message)
{
    confirmLogMessage(logOutput, kInfoLogType, loggerName, message);
}

/// @brief Requires that a log buffer contains a warning message.
/// @param[in] logOutput The captured console output or log file contents to search.
/// @param[in] loggerName The logger name expected in the message.
/// @param[in] message The message payload expected after the logger name.
inline void confirmWarningLogMessage(std::string_view logOutput, std::string_view loggerName,
                                     std::string_view message)
{
    confirmLogMessage(logOutput, kWarningLogType, loggerName, message);
}

/// @brief Requires that a log buffer contains an error message.
/// @param[in] logOutput The captured console output or log file contents to search.
/// @param[in] loggerName The logger name expected in the message.
/// @param[in] message The message payload expected after the logger name.
inline void confirmErrorLogMessage(std::string_view logOutput, std::string_view loggerName,
                                   std::string_view message)
{
    confirmLogMessage(logOutput, kErrorLogType, loggerName, message);
}

/// @brief Requires that a log buffer contains a debug message.
/// @param[in] logOutput The captured console output or log file contents to search.
/// @param[in] loggerName The logger name expected in the message.
/// @param[in] message The message payload expected after the logger name.
inline void confirmDebugLogMessage(std::string_view logOutput, std::string_view loggerName,
                                   std::string_view message)
{
    confirmLogMessage(logOutput, kDebugLogType, loggerName, message);
}

/// @brief Requires that a log buffer contains a critical message.
/// @param[in] logOutput The captured console output or log file contents to search.
/// @param[in] loggerName The logger name expected in the message.
/// @param[in] message The message payload expected after the logger name.
inline void confirmCriticalLogMessage(std::string_view logOutput, std::string_view loggerName,
                                      std::string_view message)
{
    confirmLogMessage(logOutput, kCriticalLogType, loggerName, message);
}

/// @brief Requires that a log buffer contains a trace message.
/// @param[in] logOutput The captured console output or log file contents to search.
/// @param[in] loggerName The logger name expected in the message.
/// @param[in] message The message payload expected after the logger name.
inline void confirmTraceLogMessage(std::string_view logOutput, std::string_view loggerName,
                                   std::string_view message)
{
    confirmLogMessage(logOutput, kTraceLogType, loggerName, message);
}

} // namespace Engine::Tests
