/**
 * @file  WindowSystem.hpp
 * @brief Declares engine-owned window creation, lifetime, and event polling APIs.
 */

#pragma once

#include <Engine/Core/Logger.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Engine
{

/// @brief Logical dimensions of an engine-owned window.
/// @details Values are desktop-coordinate dimensions used when asking the platform backend to
///          create a window. Window creation rejects non-positive dimensions.
struct WindowSize
{
    /// @brief Logical width in desktop units.
    int width = 1280;
    /// @brief Logical height in desktop units.
    int height = 720;
};

/// @brief Stable reference to a window managed by the Window System.
/// @details The identifier is assigned by the platform backend and wrapped in an engine-owned type
///          so callers do not need to depend on SDL window identifiers directly.
struct WindowIdentifier
{
    /// @brief Engine-owned identifier value assigned by the Window System.
    std::uint32_t value = 0;
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

/// @brief Type of window-specific event produced by the Window System.
enum class WindowEventType
{
    /// @brief The user or platform requested that a managed window close.
    CloseRequest
};

/// @brief Engine-owned description of a window-specific event.
/// @details Window events are translated from platform events and only reference windows currently
///          managed by the Window System.
struct WindowEvent
{
    /// @brief Kind of window event that was observed.
    WindowEventType type;
    /// @brief Window that emitted the event.
    WindowIdentifier windowIdentifier;
};

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

    /// @brief Polls pending platform events and returns engine-owned window results.
    /// @return Window event batch plus application-level quit state.
    /// @details If the Window System is not initialized, this returns an empty result. Events for
    ///          unmanaged windows are ignored.
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
