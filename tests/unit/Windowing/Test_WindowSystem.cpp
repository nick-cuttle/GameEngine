/**
 * @file Test_WindowSystem.cpp
 * @brief Tests public Window System concepts that do not require platform windows.
 */

#include <Engine/Windowing/WindowSystem.hpp>

#include <SDL3/SDL.h>
#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("WindowSystem", "[unit][windowing][window-system]")
{
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
        REQUIRE(SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy"));

        Engine::WindowSystem windowSystem;
        windowSystem.initialize();

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
        REQUIRE(SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy"));

        Engine::WindowSystem windowSystem;
        windowSystem.initialize();

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
        REQUIRE(pollResult.windowEvents[0].type == Engine::WindowEventType::CloseRequest);
        REQUIRE(pollResult.windowEvents[0].windowIdentifier.value == primaryWindow.value);

        windowSystem.shutdown();
    }
}
