/**
 * @file  WindowSystem.hpp
 * @brief Declares engine-owned window creation, lifetime, and event polling APIs.
 */

#pragma once

#include <Engine/Core/Logger.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace Engine
{

/// @brief Logical dimensions of an engine-owned window.
/// @details Values are desktop-coordinate dimensions used when asking the platform backend to
///          create a window. Window creation rejects non-positive dimensions.
struct WindowSize
{
    /// @brief Logical width in desktop units.
    std::uint32_t width = 1280;
    /// @brief Logical height in desktop units.
    std::uint32_t height = 720;
};

/// @brief Pixel dimensions of the drawable graphics surface attached to a window.
struct GraphicsSurfaceSize
{
    /// @brief Drawable surface width in pixels.
    std::uint32_t width = 0;
    /// @brief Drawable surface height in pixels.
    std::uint32_t height = 0;
};

/// @brief Stable reference to a window managed by the Window System.
/// @details The identifier is assigned by the platform backend and wrapped in an engine-owned type
///          so callers do not need to depend on SDL window identifiers directly.
struct WindowIdentifier
{
    /// @brief Engine-owned identifier value assigned by the Window System.
    std::uint32_t value = 0;

    /// @brief Compares two window identifiers by their engine-owned values.
    /// @param left First window identifier to compare.
    /// @param right Second window identifier to compare.
    /// @return True when both identifiers refer to the same engine-owned window.
    [[nodiscard]] friend constexpr bool operator==(WindowIdentifier const &left,
                                                  WindowIdentifier const &right) noexcept = default;
};

/// @brief Engine-owned configuration used to create a window.
/// @details This configuration is consumed by WindowSystem::createPrimaryWindow and describes the
///          initial platform window state requested by the engine.
struct WindowConfiguration
{
    /// @brief Window title displayed by the desktop environment.
    std::string title = "GameEngine";
    /// @brief Initial logical window size.
    WindowSize size{};
    /// @brief Whether the window should be visible immediately after creation.
    bool isVisible = true;
};

/// @brief Logical position of an engine-owned window.
struct WindowPosition
{
    /// @brief Horizontal position in desktop coordinates.
    std::int32_t horizontalPosition = 0;
    /// @brief Vertical position in desktop coordinates.
    std::int32_t verticalPosition = 0;
};

/// @brief Event emitted when a window receives a close request.
struct WindowCloseRequested
{
    /// @brief Window that received the close request.
    WindowIdentifier windowIdentifier;
};

/// @brief Event emitted when a window changes position.
struct WindowMoved
{
    /// @brief Window that changed position.
    WindowIdentifier windowIdentifier;
    /// @brief New window position.
    WindowPosition windowPosition;
};

/// @brief Event emitted when a window changes logical size.
struct WindowSizeChanged
{
    /// @brief Window that changed logical size.
    WindowIdentifier windowIdentifier;
    /// @brief New logical window size.
    WindowSize windowSize;
};

/// @brief Event emitted when the drawable graphics surface changes size.
struct GraphicsSurfaceSizeChanged
{
    /// @brief Window whose graphics surface changed size.
    WindowIdentifier windowIdentifier;
    /// @brief New drawable graphics surface size.
    GraphicsSurfaceSize graphicsSurfaceSize;
};

/// @brief Event emitted when a window gains input focus.
struct WindowFocusGained
{
    /// @brief Window that gained input focus.
    WindowIdentifier windowIdentifier;
};

/// @brief Event emitted when a window loses input focus.
struct WindowFocusLost
{
    /// @brief Window that lost input focus.
    WindowIdentifier windowIdentifier;
};

/// @brief Event emitted when a window is minimized.
struct WindowMinimized
{
    /// @brief Window that was minimized.
    WindowIdentifier windowIdentifier;
};

/// @brief Event emitted when a window is restored from a minimized state.
struct WindowRestored
{
    /// @brief Window that was restored.
    WindowIdentifier windowIdentifier;
};

/// @brief Event emitted when a window display scale changes.
struct WindowDisplayScaleChanged
{
    /// @brief Window whose display scale changed.
    WindowIdentifier windowIdentifier;
    /// @brief New display scale for the window.
    float displayScale = 1.0F;
};

/// @brief Engine-owned window event variant produced by the Window System.
using WindowEvent = std::variant<WindowCloseRequested, WindowMoved, WindowSizeChanged,
                                 GraphicsSurfaceSizeChanged, WindowFocusGained, WindowFocusLost,
                                 WindowMinimized, WindowRestored, WindowDisplayScaleChanged>;

/// @brief Batch of events and application-level quit state produced by event polling.
/// @details A single poll drains the platform event queue available at that time. Window-specific
///          events and process-wide quit requests are reported separately so callers can decide how
///          to handle each case.
struct WindowEventPollResult
{
    /// @brief Window-specific events observed during this poll.
    std::vector<WindowEvent> windowEvents;
    /// @brief Whether the platform requested application shutdown.
    bool isApplicationQuitRequested = false;
};

/// @brief Owns SDL video lifecycle, engine windows, and window event translation.
/// @details WindowSystem is the engine boundary around the platform windowing backend. It must be
///          initialized before creating windows, destroys all managed windows during shutdown, and
///          translates supported SDL events into engine-owned event structures.
class WindowSystem
{
public:
    /// @brief Constructs an empty Window System.
    /// @details Construction does not initialize the platform video subsystem. Call initialize()
    ///          before creating windows.
    WindowSystem();

    /// @brief Shuts down the Window System if it is still initialized.
    /// @details The destructor releases managed windows and video subsystem resources by calling
    ///          shutdown().
    ~WindowSystem();

    /// @brief Window systems are unique owners of their platform window handles.
    WindowSystem(WindowSystem const &) = delete;
    /// @brief Window systems cannot be copy-assigned because they own platform window handles.
    WindowSystem &operator=(WindowSystem const &) = delete;

    /// @brief Initializes the platform video subsystem required for windows.
    /// @details This operation is idempotent. Calling initialize() after successful initialization
    ///          has no effect.
    /// @throws std::runtime_error if the platform video subsystem cannot be initialized.
    void initialize(Logger logger);

    /// @brief Destroys managed windows and shuts down the platform video subsystem.
    /// @details This operation is idempotent. Calling shutdown() before initialization or after a
    ///          prior shutdown has no effect.
    void shutdown();

    /// @brief Creates the application Primary Window.
    /// @param configuration Engine-owned window creation settings.
    /// @return Stable identifier for the created Primary Window.
    /// @throws std::runtime_error if the Window System is not initialized or the platform backend
    ///         cannot create, identify, or present the window.
    /// @throws std::invalid_argument if the configured width or height is not greater than zero.
    WindowIdentifier createPrimaryWindow(WindowConfiguration const &configuration);

    /// @brief Drains pending platform events and returns engine-owned window results.
    /// @return Window event batch plus application-level quit state.
    /// @details If the Window System is not initialized, this returns an empty result without
    ///          polling SDL. Supported events for managed windows are translated into engine-owned
    ///          event payloads; unmanaged or unsupported events are ignored.
    WindowEventPollResult pollWindowEvents();

private:
    /// @brief SDL-specific implementation hidden from consumers of this public header.
    struct Implementation;
    /// @brief Owning pointer to the platform-specific implementation state.
    std::unique_ptr<Implementation> implementation;
    Logger m_Logger;

    /// @brief Releases resources owned by the platform-specific implementation.
    /// @details Needed to separate Logger usage from platform-specific teardown in shutdown() and
    /// the destructor.
    void releaseResources() noexcept;
};

} // namespace Engine
