/**
 * @file  InputCodes.hpp
 * @brief Defines engine-owned input codes.
 */

#pragma once

namespace Engine::Input
{

/// @brief Key codes for keyboard input events.
/// @details These values are independent of platform-specific key codes. Platform translators
///          should convert native keyboard input into these values before submitting events to the
///          Input System.
enum class KeyCode
{
    Unknown = 0,

    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    Num0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,

    Escape,
    Space,
    Enter,
    Tab,
    Backspace,

    Left,
    Right,
    Up,
    Down,

    LeftShift,
    RightShift,
    LeftControl,
    RightControl,
    LeftAlt,
    RightAlt,

    Count, // Keep last to track the number of valid keys.
};

/// @brief Mouse button codes for mouse input events.
/// @details These values are independent of platform-specific button codes. Platform translators
///          should convert native mouse input into these values before submitting events to the
///          Input System.
enum class MouseButton
{
    Unknown = 0,

    Left,
    Right,
    Middle,
    X1,
    X2,

    Count, // Keep last to track the number of mouse buttons.
};
} // namespace Engine::Input
