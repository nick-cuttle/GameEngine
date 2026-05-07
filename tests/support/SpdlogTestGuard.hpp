/**
 * @file  SpdlogTestGuard.hpp
 * @brief Defines a test guard for resetting spdlog's process-global state.
 */

#pragma once

#include <spdlog/spdlog.h>

namespace Engine::Tests
{

/// @brief Resets spdlog state before and after tests that install global loggers.
class SpdlogTestGuard
{
public:
    SpdlogTestGuard()
    {
        reset();
    }
    ~SpdlogTestGuard()
    {
        reset();
    }

    SpdlogTestGuard(SpdlogTestGuard const &) = delete;
    SpdlogTestGuard &operator=(SpdlogTestGuard const &) = delete;

private:
    /// @brief Resets spdlog state by flushing all loggers, removing all loggers, and shutting down
    static void reset()
    {
        spdlog::apply_all([](auto logger) { logger->flush(); });
        spdlog::set_default_logger(nullptr);
        spdlog::drop_all();
        spdlog::shutdown();
    }
};

} // namespace Engine::Tests
