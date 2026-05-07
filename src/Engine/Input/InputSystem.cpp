/**
 * @file  InputSystem.cpp
 * @brief Implements the input System class.
 */

#include "InputSystem.hpp"

#include <algorithm>
#include <cstddef>
#include <type_traits>

namespace Engine::Input
{
namespace
{
constexpr std::size_t index(KeyCode key)
{
    return static_cast<std::size_t>(key);
}

constexpr std::size_t index(MouseButton button)
{
    return static_cast<std::size_t>(button);
}

constexpr bool isKnown(KeyCode key)
{
    return key > KeyCode::Unknown && key < KeyCode::Count;
}

constexpr bool isKnown(MouseButton button)
{
    return button > MouseButton::Unknown && button < MouseButton::Count;
}

template <typename> constexpr bool always_false_v = false;
} // namespace

void System::beginFrame()
{
    std::ranges::fill(m_KeysPressed, false);
    std::ranges::fill(m_KeysReleased, false);
    std::ranges::fill(m_MouseButtonsPressed, false);
    std::ranges::fill(m_MouseButtonsReleased, false);
    m_MouseDelta = {};
    m_ScrollDelta = {};
}

void System::submit(Event const &event)
{
    std::visit(
        [this](auto const &input)
        {
            using Event = std::decay_t<decltype(input)>;

            // KeyPressed
            if constexpr (std::is_same_v<Event, KeyPressed>)
            {
                if (!isKnown(input.key))
                    return;

                // Only consider a key "pressed" if it wasn't already down, to avoid treating key
                // repeats as new presses.
                if (!input.repeat && !m_KeysDown[index(input.key)])
                    m_KeysPressed[index(input.key)] = true;

                m_KeysDown[index(input.key)] = true;
                m_KeysReleased[index(input.key)] = false;
            }
            // KeyReleased
            else if constexpr (std::is_same_v<Event, KeyReleased>)
            {
                if (!isKnown(input.key))
                    return;

                // Only consider a key "released" if it was previously down.
                if (m_KeysDown[index(input.key)])
                    m_KeysReleased[index(input.key)] = true;

                m_KeysDown[index(input.key)] = false;
                m_KeysPressed[index(input.key)] = false;
            }
            // MouseButtonPressed
            else if constexpr (std::is_same_v<Event, MouseButtonPressed>)
            {
                if (!isKnown(input.button))
                    return;

                // Only consider a button "pressed" if it wasn't already down, to avoid treating
                // repeats as new presses.
                if (!m_MouseButtonsDown[index(input.button)])
                    m_MouseButtonsPressed[index(input.button)] = true;

                m_MouseButtonsDown[index(input.button)] = true;
                m_MouseButtonsReleased[index(input.button)] = false;
            }
            // MouseButtonReleased
            else if constexpr (std::is_same_v<Event, MouseButtonReleased>)
            {
                if (!isKnown(input.button))
                    return;

                // Only consider a button "released" if it was previously down.
                if (m_MouseButtonsDown[index(input.button)])
                    m_MouseButtonsReleased[index(input.button)] = true;

                m_MouseButtonsDown[index(input.button)] = false;
                m_MouseButtonsPressed[index(input.button)] = false;
            }
            // MouseMoved
            else if constexpr (std::is_same_v<Event, MouseMoved>)
            {
                m_MousePosition = input.position;
                m_MouseDelta.x += input.delta.x;
                m_MouseDelta.y += input.delta.y;
            }
            // MouseWheelScrolled
            else if constexpr (std::is_same_v<Event, MouseWheelScrolled>)
            {
                m_ScrollDelta.x += input.delta.x;
                m_ScrollDelta.y += input.delta.y;
            }
            else
            {
                static_assert(always_false_v<Event>, "Unrecognized Input::Event type");
            }
        },
        event);
}

bool System::isKeyDown(KeyCode key) const
{
    if (!isKnown(key))
        return false;

    return m_KeysDown[index(key)];
}

bool System::wasKeyPressed(KeyCode key) const
{
    if (!isKnown(key))
        return false;

    return m_KeysPressed[index(key)];
}

bool System::wasKeyReleased(KeyCode key) const
{
    if (!isKnown(key))
        return false;

    return m_KeysReleased[index(key)];
}

bool System::isMouseButtonDown(MouseButton button) const
{
    if (!isKnown(button))
        return false;

    return m_MouseButtonsDown[index(button)];
}

bool System::wasMouseButtonPressed(MouseButton button) const
{
    if (!isKnown(button))
        return false;

    return m_MouseButtonsPressed[index(button)];
}

bool System::wasMouseButtonReleased(MouseButton button) const
{
    if (!isKnown(button))
        return false;

    return m_MouseButtonsReleased[index(button)];
}

MousePosition System::mousePosition() const
{
    return m_MousePosition;
}

Delta System::mouseDelta() const
{
    return m_MouseDelta;
}

Delta System::scrollDelta() const
{
    return m_ScrollDelta;
}
} // namespace Engine::Input
