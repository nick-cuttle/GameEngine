/**
 * @file Test_WindowSystem.cpp
 * @brief Tests public Window System concepts that do not require platform windows.
 */

#include <Engine/Core/Logger.hpp>
#include <Engine/Windowing/WindowSystem.hpp>

#include <SDL3/SDL.h>
#include <SpdlogTestGuard.hpp>
#include <TestTempDirectory.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <type_traits>
#include <variant>

namespace
{

/// @brief Asserts that a poll result contains one window event with the expected payload type.
/// @tparam WindowEventPayload Engine window event payload type expected in the poll result.
/// @param windowEventPollResult Poll result to inspect.
/// @return Reference to the typed payload stored in the poll result.
template <typename WindowEventPayload>
WindowEventPayload const &
requireSingleWindowEventPayload(Engine::WindowEventPollResult const &windowEventPollResult)
{
    REQUIRE_FALSE(windowEventPollResult.isApplicationQuitRequested);
    REQUIRE(windowEventPollResult.windowEvents.size() == 1);

    WindowEventPayload const *windowEventPayload =
        std::get_if<WindowEventPayload>(&windowEventPollResult.windowEvents[0]);

    REQUIRE(windowEventPayload != nullptr);

    return *windowEventPayload;
}

/// @brief Creates a hidden primary window and drains backend-created events before assertions.
/// @param windowSystem Window system that owns the test window.
/// @return Stable identifier for the created primary window.
Engine::WindowIdentifier createHiddenPrimaryWindow(Engine::WindowSystem &windowSystem)
{
    Engine::WindowConfiguration configuration;
    configuration.isVisible = false;

    Engine::WindowIdentifier primaryWindow = windowSystem.createPrimaryWindow(configuration);
    Engine::WindowEventPollResult creationPollResult = windowSystem.pollWindowEvents();

    REQUIRE_FALSE(creationPollResult.isApplicationQuitRequested);

    return primaryWindow;
}

/// @brief Pushes a synthetic SDL window event for a managed engine window.
/// @param platformEventType SDL window event type to push.
/// @param windowIdentifier Managed engine window identifier associated with the event.
/// @param firstEventValue First SDL window event data value.
/// @param secondEventValue Second SDL window event data value.
void pushManagedWindowEvent(std::uint32_t platformEventType,
                            Engine::WindowIdentifier windowIdentifier,
                            std::int32_t firstEventValue = 0, std::int32_t secondEventValue = 0)
{
    SDL_Event platformWindowEvent{};
    platformWindowEvent.type = platformEventType;
    platformWindowEvent.window.windowID = windowIdentifier.value;
    platformWindowEvent.window.data1 = firstEventValue;
    platformWindowEvent.window.data2 = secondEventValue;

    REQUIRE(SDL_PushEvent(&platformWindowEvent));
}

} // namespace

TEST_CASE("WindowSystem", "[unit][windowing][window-system]")
{
    Engine::Tests::TestTempDirectory tempDirectory("Test WindowSystem");
    Engine::Tests::SpdlogTestGuard spdlog;

    Engine::LoggingSystem loggingSystem;

    std::filesystem::path const logDir = tempDirectory.path() / "logs";

    loggingSystem.initialize({.logDirectory = logDir});
    auto windowLogger = loggingSystem.createSubsystemLogger("WindowSystem");

    // Select a display-independent video backend before WindowSystem initializes SDL.
    REQUIRE(SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "dummy", SDL_HINT_OVERRIDE));

    Engine::WindowSystem windowSystem;
    windowSystem.initialize(windowLogger);

    SECTION("primary window configuration uses issue 20 defaults")
    {
        Engine::WindowConfiguration configuration;

        REQUIRE(configuration.title == std::string{"GameEngine"});
        REQUIRE(configuration.size.width == 1280);
        REQUIRE(configuration.size.height == 720);
        REQUIRE(configuration.isVisible);
    }

    SECTION("poll result starts empty")
    {
        Engine::WindowEventPollResult pollResult;

        REQUIRE(pollResult.windowEvents.empty());
        REQUIRE_FALSE(pollResult.isApplicationQuitRequested);
    }

    SECTION("poll result reports application quit requests")
    {
        SDL_Event quitEvent{};
        quitEvent.type = SDL_EVENT_QUIT;

        REQUIRE(SDL_PushEvent(&quitEvent));

        Engine::WindowEventPollResult pollResult = windowSystem.pollWindowEvents();

        REQUIRE(pollResult.isApplicationQuitRequested);
        REQUIRE(pollResult.windowEvents.empty());

        windowSystem.shutdown();
    }

    SECTION("poll result reports primary window close requests")
    {
        Engine::WindowConfiguration configuration;
        configuration.isVisible = false;

        Engine::WindowIdentifier primaryWindow = windowSystem.createPrimaryWindow(configuration);
        Engine::WindowEventPollResult creationPollResult = windowSystem.pollWindowEvents();

        REQUIRE_FALSE(creationPollResult.isApplicationQuitRequested);

        SDL_Event closeRequestEvent{};
        closeRequestEvent.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
        closeRequestEvent.window.windowID = primaryWindow.value;

        REQUIRE(SDL_PushEvent(&closeRequestEvent));

        Engine::WindowEventPollResult pollResult = windowSystem.pollWindowEvents();

        REQUIRE_FALSE(pollResult.isApplicationQuitRequested);
        REQUIRE(pollResult.windowEvents.size() == 1);

        Engine::WindowCloseRequested const *windowCloseRequested =
            std::get_if<Engine::WindowCloseRequested>(&pollResult.windowEvents[0]);

        REQUIRE(windowCloseRequested != nullptr);
        REQUIRE(windowCloseRequested->windowIdentifier.value == primaryWindow.value);

        windowSystem.shutdown();
    }

    SECTION("window size uses explicit unsigned dimensions")
    {
        STATIC_REQUIRE((std::is_same_v<decltype(Engine::WindowSize{}.width), std::uint32_t>));
        STATIC_REQUIRE((std::is_same_v<decltype(Engine::WindowSize{}.height), std::uint32_t>));
    }

    SECTION("graphics surface size is distinct from window size")
    {
        STATIC_REQUIRE_FALSE((std::is_same_v<Engine::GraphicsSurfaceSize, Engine::WindowSize>));
        STATIC_REQUIRE(
            (std::is_same_v<decltype(Engine::GraphicsSurfaceSize{}.width), std::uint32_t>));
        STATIC_REQUIRE(
            (std::is_same_v<decltype(Engine::GraphicsSurfaceSize{}.height), std::uint32_t>));
    }

    SECTION("window position supports signed coordinates")
    {
        STATIC_REQUIRE(
            (std::is_same_v<decltype(Engine::WindowPosition{}.horizontalPosition), std::int32_t>));
        STATIC_REQUIRE(
            (std::is_same_v<decltype(Engine::WindowPosition{}.verticalPosition), std::int32_t>));

        Engine::WindowPosition windowPosition{.horizontalPosition = -320, .verticalPosition = -240};

        REQUIRE(windowPosition.horizontalPosition == -320);
        REQUIRE(windowPosition.verticalPosition == -240);
    }

    SECTION("window event payloads are strongly typed")
    {
        STATIC_REQUIRE(std::variant_size_v<Engine::WindowEvent> == 9);
        STATIC_REQUIRE((std::is_same_v<std::variant_alternative_t<0, Engine::WindowEvent>,
                                       Engine::WindowCloseRequested>));
        STATIC_REQUIRE((std::is_same_v<std::variant_alternative_t<1, Engine::WindowEvent>,
                                       Engine::WindowMoved>));
        STATIC_REQUIRE((std::is_same_v<std::variant_alternative_t<2, Engine::WindowEvent>,
                                       Engine::WindowSizeChanged>));
        STATIC_REQUIRE((std::is_same_v<std::variant_alternative_t<3, Engine::WindowEvent>,
                                       Engine::GraphicsSurfaceSizeChanged>));
        STATIC_REQUIRE((std::is_same_v<std::variant_alternative_t<4, Engine::WindowEvent>,
                                       Engine::WindowFocusGained>));
        STATIC_REQUIRE((std::is_same_v<std::variant_alternative_t<5, Engine::WindowEvent>,
                                       Engine::WindowFocusLost>));
        STATIC_REQUIRE((std::is_same_v<std::variant_alternative_t<6, Engine::WindowEvent>,
                                       Engine::WindowMinimized>));
        STATIC_REQUIRE((std::is_same_v<std::variant_alternative_t<7, Engine::WindowEvent>,
                                       Engine::WindowRestored>));
        STATIC_REQUIRE((std::is_same_v<std::variant_alternative_t<8, Engine::WindowEvent>,
                                       Engine::WindowDisplayScaleChanged>));
    }

    SECTION("poll result reports window movement")
    {
        Engine::WindowIdentifier primaryWindow = createHiddenPrimaryWindow(windowSystem);

        pushManagedWindowEvent(SDL_EVENT_WINDOW_MOVED, primaryWindow, -15, 24);

        Engine::WindowEventPollResult pollResult = windowSystem.pollWindowEvents();
        Engine::WindowMoved const &windowMoved =
            requireSingleWindowEventPayload<Engine::WindowMoved>(pollResult);

        REQUIRE(windowMoved.windowIdentifier.value == primaryWindow.value);
        REQUIRE(windowMoved.windowPosition.horizontalPosition == -15);
        REQUIRE(windowMoved.windowPosition.verticalPosition == 24);
    }

    SECTION("poll result reports window size changes")
    {
        Engine::WindowIdentifier primaryWindow = createHiddenPrimaryWindow(windowSystem);

        pushManagedWindowEvent(SDL_EVENT_WINDOW_RESIZED, primaryWindow, 1600, 900);

        Engine::WindowEventPollResult pollResult = windowSystem.pollWindowEvents();
        Engine::WindowSizeChanged const &windowSizeChanged =
            requireSingleWindowEventPayload<Engine::WindowSizeChanged>(pollResult);

        REQUIRE(windowSizeChanged.windowIdentifier.value == primaryWindow.value);
        REQUIRE(windowSizeChanged.windowSize.width == 1600);
        REQUIRE(windowSizeChanged.windowSize.height == 900);
    }

    SECTION("poll result reports graphics surface size changes")
    {
        Engine::WindowIdentifier primaryWindow = createHiddenPrimaryWindow(windowSystem);

        pushManagedWindowEvent(SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED, primaryWindow, 3200, 1800);

        Engine::WindowEventPollResult pollResult = windowSystem.pollWindowEvents();
        Engine::GraphicsSurfaceSizeChanged const &graphicsSurfaceSizeChanged =
            requireSingleWindowEventPayload<Engine::GraphicsSurfaceSizeChanged>(pollResult);

        REQUIRE(graphicsSurfaceSizeChanged.windowIdentifier.value == primaryWindow.value);
        REQUIRE(graphicsSurfaceSizeChanged.graphicsSurfaceSize.width == 3200);
        REQUIRE(graphicsSurfaceSizeChanged.graphicsSurfaceSize.height == 1800);
    }

    SECTION("poll result reports focus changes")
    {
        Engine::WindowIdentifier primaryWindow = createHiddenPrimaryWindow(windowSystem);

        pushManagedWindowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED, primaryWindow);
        pushManagedWindowEvent(SDL_EVENT_WINDOW_FOCUS_LOST, primaryWindow);

        Engine::WindowEventPollResult pollResult = windowSystem.pollWindowEvents();

        REQUIRE_FALSE(pollResult.isApplicationQuitRequested);
        REQUIRE(pollResult.windowEvents.size() == 2);

        Engine::WindowFocusGained const *windowFocusGained =
            std::get_if<Engine::WindowFocusGained>(&pollResult.windowEvents[0]);
        Engine::WindowFocusLost const *windowFocusLost =
            std::get_if<Engine::WindowFocusLost>(&pollResult.windowEvents[1]);

        REQUIRE(windowFocusGained != nullptr);
        REQUIRE(windowFocusLost != nullptr);
        REQUIRE(windowFocusGained->windowIdentifier.value == primaryWindow.value);
        REQUIRE(windowFocusLost->windowIdentifier.value == primaryWindow.value);
    }

    SECTION("poll result reports minimized and restored state")
    {
        Engine::WindowIdentifier primaryWindow = createHiddenPrimaryWindow(windowSystem);

        pushManagedWindowEvent(SDL_EVENT_WINDOW_MINIMIZED, primaryWindow);
        pushManagedWindowEvent(SDL_EVENT_WINDOW_RESTORED, primaryWindow);

        Engine::WindowEventPollResult pollResult = windowSystem.pollWindowEvents();

        REQUIRE_FALSE(pollResult.isApplicationQuitRequested);
        REQUIRE(pollResult.windowEvents.size() == 2);

        Engine::WindowMinimized const *windowMinimized =
            std::get_if<Engine::WindowMinimized>(&pollResult.windowEvents[0]);
        Engine::WindowRestored const *windowRestored =
            std::get_if<Engine::WindowRestored>(&pollResult.windowEvents[1]);

        REQUIRE(windowMinimized != nullptr);
        REQUIRE(windowRestored != nullptr);
        REQUIRE(windowMinimized->windowIdentifier.value == primaryWindow.value);
        REQUIRE(windowRestored->windowIdentifier.value == primaryWindow.value);
    }

    SECTION("poll result reports display scale changes")
    {
        Engine::WindowIdentifier primaryWindow = createHiddenPrimaryWindow(windowSystem);

        pushManagedWindowEvent(SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED, primaryWindow);

        Engine::WindowEventPollResult pollResult = windowSystem.pollWindowEvents();
        Engine::WindowDisplayScaleChanged const &windowDisplayScaleChanged =
            requireSingleWindowEventPayload<Engine::WindowDisplayScaleChanged>(pollResult);

        REQUIRE(windowDisplayScaleChanged.windowIdentifier.value == primaryWindow.value);
        REQUIRE(windowDisplayScaleChanged.displayScale > 0.0F);
    }

    SECTION("poll result ignores non-window events")
    {
        Engine::WindowEventPollResult initialPollResult = windowSystem.pollWindowEvents();

        REQUIRE_FALSE(initialPollResult.isApplicationQuitRequested);
        REQUIRE(initialPollResult.windowEvents.empty());

        SDL_Event keyboardEvent{};
        keyboardEvent.type = SDL_EVENT_KEY_DOWN;

        REQUIRE(SDL_PushEvent(&keyboardEvent));

        Engine::WindowEventPollResult pollResult = windowSystem.pollWindowEvents();

        REQUIRE_FALSE(pollResult.isApplicationQuitRequested);
        REQUIRE(pollResult.windowEvents.empty());
    }

    windowSystem.shutdown();
    loggingSystem.shutdown();
}
