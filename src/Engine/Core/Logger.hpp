/**
 * @file  Logger.hpp
 * @brief Defines the Logger class, which manages logging functionality for the engine.
 */

#pragma once

#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>

namespace Engine
{
class Logger
{
  public:
    struct Config
    {
        std::filesystem::path logDirectory; // Directory where log files will be stored
        bool async = false;
    };

    /// @brief Initializes the logger with the specified configuration.
    /// @param[in] config The configuration for the logger.
    void initialize(Config const &config);

    /// @brief Gets the root logger instance from which all subsystem loggers are derived.
    /// @details This is the base Engine logger.
    /// @return The root logger instance.
    std::shared_ptr<spdlog::logger> root() const;

    /// @brief Creates a subsystem logger derived from the root logger.
    /// @param[in] name The name of the subsystem logger.
    /// @return The created subsystem logger instance.
    std::shared_ptr<spdlog::logger> createSubsystem(std::string const &name) const;

  private:
    std::shared_ptr<spdlog::logger> m_RootLogger;
};
} // namespace Engine