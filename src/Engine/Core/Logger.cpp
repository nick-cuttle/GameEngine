/**
 * @file  Logger.cpp
 * @brief Implements the Logger class, which manages logging functionality for the engine.
 */

#include "Logger.hpp"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>

namespace Engine
{

void Logger::initialize(Config const &config)
{
    std::filesystem::create_directories(config.logDirectory);

    auto const logFile = config.logDirectory / "Engine.log";

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
    if (config.async)
    {
        spdlog::init_thread_pool(8192, 1);

        m_RootLogger = std::make_shared<spdlog::async_logger>(
            "Engine", sinks.begin(), sinks.end(), spdlog::thread_pool(),
            spdlog::async_overflow_policy::overrun_oldest);
    }
    else
    {
        m_RootLogger = std::make_shared<spdlog::logger>("Engine", sinks.begin(), sinks.end());
    }

    // -------------------------
    // Log levels
    // -------------------------
#ifdef NDEBUG
    m_RootLogger->set_level(spdlog::level::info);
#else
    m_RootLogger->set_level(spdlog::level::trace);
#endif

    m_RootLogger->flush_on(spdlog::level::warn);
    spdlog::flush_every(std::chrono::seconds(1));

    // IMPORTANT: engine logger is the default sink
    spdlog::set_default_logger(m_RootLogger);

    m_RootLogger->info("Engine logger initialized");
    m_RootLogger->info("Log file: {}", logFile.string());
}

std::shared_ptr<spdlog::logger> Logger::root() const { return m_RootLogger; }

std::shared_ptr<spdlog::logger> Logger::createSubsystem(std::string const &name) const
{
    if (spdlog::get(name))
        throw std::runtime_error("Logger with name '" + name + "' already exists");

    auto logger = m_RootLogger->clone(name);
    logger->set_level(m_RootLogger->level());

    spdlog::register_logger(logger);

    logger->info("Subsystem logger '{}' created", name);

    return logger;
}

} // namespace Engine