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

    // intialize logger
    Logger::Config loggerConfig;
    loggerConfig.logDirectory = paths.logs();
    logger.init(loggerConfig);
}

} // namespace Engine
