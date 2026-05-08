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

    LoggingConfiguration loggingConfiguration;
    loggingConfiguration.logDirectory = paths.logs();
    loggingSystem.initialize(loggingConfiguration);
}

} // namespace Engine
