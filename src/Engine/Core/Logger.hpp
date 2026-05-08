/**
 * @file  Logger.hpp
 * @brief Defines engine-owned logging handles and backend ownership.
 */

#pragma once

#include <fmt/format.h>

#include <filesystem>
#include <memory>
#include <string_view>
#include <utility>

namespace Engine
{

/// @brief Configuration used to initialize the engine logging backend.
struct LoggingConfiguration
{
    /// @brief Directory where engine log files will be stored.
    std::filesystem::path logDirectory;
    /// @brief Whether backend logging should use asynchronous dispatch.
    bool async = false;
};

/// @brief Lightweight engine-owned handle for writing log messages.
class Logger
{
public:
    /// @brief Constructs an invalid no-op logger handle.
    Logger() = default;

    /// @brief Writes a trace-level message if this handle references a logger.
    void trace(std::string_view message) const;
    /// @brief Formats and writes a trace-level message if this handle references a logger.
    template <typename... Arguments>
    void trace(fmt::format_string<Arguments...> messageFormat, Arguments &&...arguments) const
    {
        trace(fmt::format(messageFormat, std::forward<Arguments>(arguments)...));
    }

    /// @brief Writes a debug-level message if this handle references a logger.
    void debug(std::string_view message) const;
    /// @brief Formats and writes a debug-level message if this handle references a logger.
    template <typename... Arguments>
    void debug(fmt::format_string<Arguments...> messageFormat, Arguments &&...arguments) const
    {
        debug(fmt::format(messageFormat, std::forward<Arguments>(arguments)...));
    }

    /// @brief Writes an info-level message if this handle references a logger.
    void info(std::string_view message) const;
    /// @brief Formats and writes an info-level message if this handle references a logger.
    template <typename... Arguments>
    void info(fmt::format_string<Arguments...> messageFormat, Arguments &&...arguments) const
    {
        info(fmt::format(messageFormat, std::forward<Arguments>(arguments)...));
    }

    /// @brief Writes a warning-level message if this handle references a logger.
    void warn(std::string_view message) const;
    /// @brief Formats and writes a warning-level message if this handle references a logger.
    template <typename... Arguments>
    void warn(fmt::format_string<Arguments...> messageFormat, Arguments &&...arguments) const
    {
        warn(fmt::format(messageFormat, std::forward<Arguments>(arguments)...));
    }

    /// @brief Writes an error-level message if this handle references a logger.
    void error(std::string_view message) const;
    /// @brief Formats and writes an error-level message if this handle references a logger.
    template <typename... Arguments>
    void error(fmt::format_string<Arguments...> messageFormat, Arguments &&...arguments) const
    {
        error(fmt::format(messageFormat, std::forward<Arguments>(arguments)...));
    }

    /// @brief Writes a critical-level message if this handle references a logger.
    void critical(std::string_view message) const;
    /// @brief Formats and writes a critical-level message if this handle references a logger.
    template <typename... Arguments>
    void critical(fmt::format_string<Arguments...> messageFormat, Arguments &&...arguments) const
    {
        critical(fmt::format(messageFormat, std::forward<Arguments>(arguments)...));
    }

    /// @brief Flushes this logger if this handle references one.
    void flush() const;

private:
    friend class LoggingSystem;

    /// @brief Implementation hidden from consumers of this public header.
    /// @details hides spdlog logger from header.
    struct Implementation;

    explicit Logger(std::shared_ptr<Implementation> implementation);

    std::shared_ptr<Implementation> implementation;
};

/// @brief Owns engine logging backend setup and creates engine-owned logger handles.
class LoggingSystem
{
public:
    /// @brief Initializes the logging backend using the specified configuration.
    /// @throws std::runtime_error if the backend has already been initialized.
    void initialize(LoggingConfiguration const &configuration);

    /// @brief Gets the root Engine logger handle.
    /// @throws std::runtime_error if the logging backend is not initialized.
    Logger root() const;

    /// @brief Creates a subsystem logger derived from the root Engine logger.
    /// @param[in] subsystemName PascalCase subsystem name, such as WindowSystem.
    /// @return Lightweight handle for the created subsystem logger.
    /// @throws std::runtime_error if the backend is not initialized or the name already exists.
    Logger createSubsystemLogger(std::string_view subsystemName) const;

    /// @brief Flushes all registered backend loggers.
    void flush() const;

    /// @brief Releases backend logger state and resets process-global logging state.
    void shutdown();

private:
    struct BackendState;

    std::shared_ptr<BackendState> backendState;
};
} // namespace Engine
