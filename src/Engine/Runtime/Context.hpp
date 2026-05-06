/**
 * @file  Context.hpp
 * @brief Defines the Context class, which serves as a central point for managing engine-wide
 *        resources and configurations.
 *
 *
 */

#pragma once

#include <Engine/Core/Logger.hpp>
#include <Engine/Core/Paths.hpp>

namespace Engine
{
/// @brief Manages configurations and context for the engine
class Context
{
  public:
    /// @brief Initializes the engine context, including paths.
    void initialize();

    /// @brief Engine-wide filesystem paths initialized during context startup.
    Paths paths;
    /// @brief Engine-wide logger initialized during context startup.
    Logger logger;
};
} // namespace Engine
