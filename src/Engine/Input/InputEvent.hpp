/**
 * @file  InputEvent.hpp
 * @brief Defines engine-owned input events.
 */

#pragma once

#include <Engine/Input/InputCodes.hpp>

#include <variant>

namespace Engine::Input
{

/// @brief Engine-owned input events. These are the only events that can be submitted to the input
struct MousePosition
{
    float x = 0.0F;
    float y = 0.0F;
};

/// @brief Relative movement or scroll delta for mouse movement and wheel scroll events.
struct Delta
{
    float x = 0.0F;
    float y = 0.0F;
};

struct KeyPressed
{
    KeyCode key = KeyCode::Unknown;
    bool repeat = false;
};

struct KeyReleased
{
    KeyCode key = KeyCode::Unknown;
};

//
struct MouseButtonPressed
{
    MouseButton button = MouseButton::Unknown;
};

struct MouseButtonReleased
{
    MouseButton button = MouseButton::Unknown;
};

struct MouseMoved
{
    MousePosition position;
    Delta delta;
};

struct MouseWheelScrolled
{
    Delta delta;
};

using Event = std::variant<KeyPressed, KeyReleased, MouseButtonPressed, MouseButtonReleased,
                           MouseMoved, MouseWheelScrolled>;
} // namespace Engine::Input
