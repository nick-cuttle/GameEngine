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

    windowSystem.shutdown();
    loggingSystem.shutdown();
}
