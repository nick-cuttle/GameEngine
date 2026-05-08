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

#include <string>
#include <variant>

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
        // TODO: Assert WindowSize width and height use std::uint32_t.}
    }

    SECTION("graphics surface size is distinct from window size")
    {
        // TODO: Assert GraphicsSurfaceSize exists as a separate public type from WindowSize.
    }

    SECTION("window position supports signed coordinates")
    {
        // TODO: Assert WindowPosition coordinates can represent negative values.
    }

    SECTION("window event payloads are strongly typed")
    {
        // TODO: Assert WindowEvent is a variant of specific payload types, not a type enum plus
        // shared fields.
    }

    SECTION("poll result reports window movement")
    {
        // TODO: Push SDL_EVENT_WINDOW_MOVED and assert WindowMoved payload.
    }

    SECTION("poll result reports window size changes")
    {
        // TODO: Push SDL_EVENT_WINDOW_RESIZED and assert WindowSizeChanged payload.
    }

    SECTION("poll result reports graphics surface size changes")
    {
        // TODO: Push SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED and assert GraphicsSurfaceSizeChanged
        // payload.
    }

    SECTION("poll result reports focus changes")
    {
        // TODO: Push focus gained and lost events and assert matching payloads.
    }

    SECTION("poll result reports minimized and restored state")
    {
        // TODO: Push minimized and restored events and assert matching payloads.
    }

    SECTION("poll result reports display scale changes")
    {
        // TODO: Push SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED and assert
        // WindowDisplayScaleChanged payload.
    }

    SECTION("poll result ignores non-window events")
    {
        // TODO: Push an SDL event that should not become an Engine::WindowEvent.
    }

    windowSystem.shutdown();
    loggingSystem.shutdown();
}
