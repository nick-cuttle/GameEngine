/**
 * @file  Logger.cpp
 * @brief Implements engine-owned logging handles and backend ownership.
 */

#include "Logger.hpp"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Engine
{

namespace
{

constexpr auto kRootLoggerName = "Engine";
constexpr auto kLogFileName = "Engine.log";

} // namespace

struct Logger::Implementation
{
    explicit Implementation(std::shared_ptr<spdlog::logger> logger) : logger(std::move(logger))
    {
    }

    std::shared_ptr<spdlog::logger> logger;
};

struct LoggingSystem::BackendState
{
    std::shared_ptr<spdlog::logger> rootLogger;
};

Logger::Logger(std::shared_ptr<Implementation> implementation) :
    implementation(std::move(implementation))
{
}

void Logger::trace(std::string_view message) const
{
    if (implementation && implementation->logger)
    {
        implementation->logger->trace("{}", message);
    }
}

void Logger::debug(std::string_view message) const
{
    if (implementation && implementation->logger)
    {
        implementation->logger->debug("{}", message);
    }
}

void Logger::info(std::string_view message) const
{
    if (implementation && implementation->logger)
    {
        implementation->logger->info("{}", message);
    }
}

void Logger::warn(std::string_view message) const
{
    if (implementation && implementation->logger)
    {
        implementation->logger->warn("{}", message);
    }
}

void Logger::error(std::string_view message) const
{
    if (implementation && implementation->logger)
    {
        implementation->logger->error("{}", message);
    }
}

void Logger::critical(std::string_view message) const
{
    if (implementation && implementation->logger)
    {
        implementation->logger->critical("{}", message);
    }
}

void Logger::flush() const
{
    if (implementation && implementation->logger)
    {
        implementation->logger->flush();
    }
}

void LoggingSystem::initialize(LoggingConfiguration const &configuration)
{
    if (backendState && backendState->rootLogger)
    {
        throw std::runtime_error("LoggingSystem has already been initialized");
    }

    std::filesystem::create_directories(configuration.logDirectory);

    auto const logFile = configuration.logDirectory / kLogFileName;

    std::vector<spdlog::sink_ptr> sinks;

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_pattern("[%T] [%^%l%$] [%n] %v");

    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile.string(), true);
    fileSink->set_pattern("[%Y-%m-%d %T.%e] [%l] [%n] %v");

    sinks.push_back(consoleSink);
    sinks.push_back(fileSink);

    // -------------------------
    // Root Engine Logger
    // -------------------------
    auto state = std::make_shared<BackendState>();

    if (configuration.async)
    {
        spdlog::init_thread_pool(8192, 1);

        state->rootLogger = std::make_shared<spdlog::async_logger>(
            kRootLoggerName, sinks.begin(), sinks.end(), spdlog::thread_pool(),
            spdlog::async_overflow_policy::overrun_oldest);
    }
    else
    {
        state->rootLogger =
            std::make_shared<spdlog::logger>(kRootLoggerName, sinks.begin(), sinks.end());
    }

    // -------------------------
    // Log levels
    // -------------------------
#ifdef NDEBUG
    state->rootLogger->set_level(spdlog::level::info);
#else
    state->rootLogger->set_level(spdlog::level::trace);
#endif

    state->rootLogger->flush_on(spdlog::level::warn);
    spdlog::flush_every(std::chrono::seconds(1));

    // IMPORTANT: engine logger is the default sink
    spdlog::set_default_logger(state->rootLogger);

    backendState = std::move(state);

    backendState->rootLogger->info("Engine logger initialized");
    backendState->rootLogger->info("Log file: {}", logFile.string());
}

Logger LoggingSystem::root() const
{
    if (!backendState || !backendState->rootLogger)
    {
        throw std::runtime_error("LoggingSystem must be initialized before requesting root logger");
    }

    return Logger(std::make_shared<Logger::Implementation>(backendState->rootLogger));
}

Logger LoggingSystem::createSubsystemLogger(std::string_view subsystemName) const
{
    if (!backendState || !backendState->rootLogger)
    {
        throw std::runtime_error(
            "LoggingSystem must be initialized before creating subsystem loggers");
    }

    std::string const name{subsystemName};

    if (spdlog::get(name))
    {
        throw std::runtime_error("Logger with name '" + name + "' already exists");
    }

    auto logger = backendState->rootLogger->clone(name);
    logger->set_level(backendState->rootLogger->level());

    spdlog::register_logger(logger);

    logger->info("Subsystem logger '{}' created", name);

    return Logger(std::make_shared<Logger::Implementation>(std::move(logger)));
}

void LoggingSystem::flush() const
{
    spdlog::apply_all([](auto logger) { logger->flush(); });
}

void LoggingSystem::shutdown()
{
    flush();
    backendState.reset();
    spdlog::set_default_logger(nullptr);
    spdlog::drop_all();
    spdlog::shutdown();
}

} // namespace Engine
