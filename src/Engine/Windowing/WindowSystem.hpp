#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Engine
{

/// @brief Logical dimensions of an engine-owned window.
struct WindowSize
{
    /// @brief Logical width in desktop units.
    int width = 1280;
    /// @brief Logical height in desktop units.
    int height = 720;
};

/// @brief Stable reference to a window managed by the Window System.
struct WindowIdentifier
{
    /// @brief Engine-owned identifier value assigned by the Window System.
    std::uint32_t value = 0;
};

/// @brief Engine-owned configuration used to create a window.
struct WindowConfiguration
{
    /// @brief Window title displayed by the desktop environment.
    std::string title = "GameEngine";
    /// @brief Initial logical Window Size.
    WindowSize size{};
    /// @brief Whether the window should be visible immediately after creation.
    bool isVisible = true;
};

/// @brief Type of window-specific event produced by the Window System.
enum class WindowEventType
{
    CloseRequest
};

/// @brief Engine-owned description of a window-specific event.
struct WindowEvent
{
    /// @brief Kind of window event that was observed.
    WindowEventType type;
    /// @brief Window that emitted the event.
    WindowIdentifier windowIdentifier;
};

/// @brief Batch of events and application-level quit state produced by event polling.
struct WindowEventPollResult
{
    /// @brief Window-specific events observed during this poll.
    std::vector<WindowEvent> windowEvents;
    /// @brief Whether the platform requested application shutdown.
    bool isApplicationQuitRequested = false;
};

/// @brief Owns SDL video lifecycle, engine windows, and window event translation.
class WindowSystem
{
  public:
    /// @brief Constructs an empty Window System.
    WindowSystem();

    /// @brief Shuts down the Window System if it is still initialized.
    ~WindowSystem();

    WindowSystem(WindowSystem const &) = delete;
    WindowSystem &operator=(WindowSystem const &) = delete;

    /// @brief Initializes the platform video subsystem required for windows.
    void initialize();

    /// @brief Destroys managed windows and shuts down the platform video subsystem.
    void shutdown();

    /// @brief Creates the application Primary Window.
    /// @param configuration Engine-owned window creation settings.
    /// @return Stable identifier for the created Primary Window.
    WindowIdentifier createPrimaryWindow(WindowConfiguration const &configuration);

    /// @brief Polls pending platform events and returns engine-owned window results.
    /// @return Window event batch plus application-level quit state.
    WindowEventPollResult pollWindowEvents();

  private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation;
};

} // namespace Engine
