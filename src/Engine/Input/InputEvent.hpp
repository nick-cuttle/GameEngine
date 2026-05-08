/**
 * @file  InputEvent.hpp
 * @brief Defines engine-owned input events.
 */

#pragma once

#include <Engine/Input/InputCodes.hpp>

#include <variant>

namespace Engine::Input
{

/// @brief Absolute mouse position in engine input coordinates.
struct MousePosition
{
    /// @brief Horizontal mouse position.
    float x = 0.0F;
    /// @brief Vertical mouse position.
    float y = 0.0F;
};

/// @brief Relative movement or scroll delta for mouse movement and wheel scroll events.
struct Delta
{
    /// @brief Horizontal movement or wheel delta.
    float x = 0.0F;
    /// @brief Vertical movement or wheel delta.
    float y = 0.0F;
};

/// @brief Event emitted when a keyboard key is pressed.
struct KeyPressed
{
    /// @brief Engine-owned key that was pressed.
    KeyCode key = KeyCode::Unknown;
    /// @brief Whether this event is an explicit platform key-repeat event.
    bool repeat = false;
};

/// @brief Event emitted when a keyboard key is released.
struct KeyReleased
{
    /// @brief Engine-owned key that was released.
    KeyCode key = KeyCode::Unknown;
};

/// @brief Event emitted when a mouse button is pressed.
struct MouseButtonPressed
{
    /// @brief Engine-owned mouse button that was pressed.
    MouseButton button = MouseButton::Unknown;
};

/// @brief Event emitted when a mouse button is released.
struct MouseButtonReleased
{
    /// @brief Engine-owned mouse button that was released.
    MouseButton button = MouseButton::Unknown;
};

/// @brief Event emitted when the mouse moves.
struct MouseMoved
{
    /// @brief Latest absolute mouse position.
    MousePosition position;
    /// @brief Relative movement since the prior mouse position event.
    Delta delta;
};

/// @brief Event emitted when the mouse wheel scrolls.
struct MouseWheelScrolled
{
    /// @brief Relative wheel movement.
    Delta delta;
};

/// @brief Variant of all engine-owned input events accepted by Input::System.
using Event = std::variant<KeyPressed, KeyReleased, MouseButtonPressed, MouseButtonReleased,
                           MouseMoved, MouseWheelScrolled>;
} // namespace Engine::Input
