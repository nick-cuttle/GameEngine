/**
 * @file  InputSystem.hpp
 * @brief Defines the input System class, which tracks frame-based raw input state.
 */

#pragma once

#include <Engine/Input/InputEvent.hpp>

#include <array>
#include <cstddef>

namespace Engine::Input
{
class System
{
public:
    /// @brief Starts a new input frame.
    /// @details Clears transient state such as pressed/released flags, mouse movement delta, and
    /// scroll delta while preserving held key/button state and the current mouse position.
    void beginFrame();

    /// @brief Applies an engine-owned input event to the current input state.
    /// @details Invalid key/button values are ignored. Press events update held state and may set
    /// the current-frame pressed flag. Release events clear held state and may set the
    /// current-frame released flag. Movement and scroll events accumulate deltas for the active
    /// frame.
    void submit(Event const &event);

    /// @brief Returns whether a key is currently held down.
    /// @details Invalid keys, including Unknown and Count, always return false.
    bool isKeyDown(KeyCode key) const;

    /// @brief Returns whether a key became pressed during the current frame.
    /// @details This flag is cleared by beginFrame. Explicit repeat key events do not set it.
    bool wasKeyPressed(KeyCode key) const;

    /// @brief Returns whether a key became released during the current frame.
    /// @details This flag is cleared by beginFrame and is only set when a previously held key is
    /// released.
    bool wasKeyReleased(KeyCode key) const;

    /// @brief Returns whether a mouse button is currently held down.
    /// @details Invalid buttons, including Unknown and Count, always return false.
    bool isMouseButtonDown(MouseButton button) const;

    /// @brief Returns whether a mouse button became pressed during the current frame.
    /// @details This flag is cleared by beginFrame and repeated press events while held do not set
    /// it again.
    bool wasMouseButtonPressed(MouseButton button) const;

    /// @brief Returns whether a mouse button became released during the current frame.
    /// @details This flag is cleared by beginFrame and is only set when a previously held button is
    /// released.
    bool wasMouseButtonReleased(MouseButton button) const;

    /// @brief Returns the latest known mouse position.
    /// @details The position persists across frames until a MouseMoved event updates it.
    MousePosition mousePosition() const;

    /// @brief Returns accumulated mouse movement for the current frame.
    /// @details Multiple MouseMoved events in one frame are summed. The value is reset by
    /// beginFrame.
    Delta mouseDelta() const;

    /// @brief Returns accumulated mouse wheel movement for the current frame.
    /// @details Multiple MouseWheelScrolled events in one frame are summed. The value is reset by
    /// beginFrame.
    Delta scrollDelta() const;

private:
    static constexpr auto kKeyCount = static_cast<std::size_t>(KeyCode::Count);
    static constexpr auto kMouseButtonCount = static_cast<std::size_t>(MouseButton::Count);

    std::array<bool, kKeyCount> m_KeysDown = {};
    std::array<bool, kKeyCount> m_KeysPressed = {};
    std::array<bool, kKeyCount> m_KeysReleased = {};

    std::array<bool, kMouseButtonCount> m_MouseButtonsDown = {};
    std::array<bool, kMouseButtonCount> m_MouseButtonsPressed = {};
    std::array<bool, kMouseButtonCount> m_MouseButtonsReleased = {};

    MousePosition m_MousePosition;
    Delta m_MouseDelta;
    Delta m_ScrollDelta;
};
} // namespace Engine::Input
