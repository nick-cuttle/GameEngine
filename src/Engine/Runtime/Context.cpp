/**
 * @file  Context.cpp
 * @brief Implements the Context class, which serves as a central point for managing engine-wide
 *
 *
 */

#include "Context.hpp"

namespace Engine
{

void Context::initialize()
{
    paths.initialize();

    // initialize logger
    Logger::Config loggerConfig;
    loggerConfig.logDirectory = paths.logs();
    logger.initialize(loggerConfig);
}

} // namespace Engine
