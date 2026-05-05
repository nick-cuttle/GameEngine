/**
 * @file  Context.hpp
 * @brief Defines the Context class, which serves as a central point for managing engine-wide
 *        resources and configurations.
 *
 *
 */

#pragma once

#include <Engine/Core/Paths.hpp>

namespace Engine
{
/// @brief Manages configurations and context for the engine
class Context
{
  public:
    /// @brief Initializes the engine context, including paths.
    void Init();

    Paths Paths;
    /// @todo Add logging here
};
} // namespace Engine