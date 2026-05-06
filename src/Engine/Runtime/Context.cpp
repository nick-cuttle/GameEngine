/**
 * @file  Context.cpp
 * @brief Implements the Context class, which serves as a central point for managing engine-wide
 *
 *
 */

#include "Context.hpp"

namespace Engine
{

void Context::Init()
{
    // initialize paths.
    EnginePaths.Init();
}

} // namespace Engine