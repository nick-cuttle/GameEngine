/**
 * @file  main.cpp
 * @brief Entry point for the GameEngine.
 *
 * Handles initialization and main loop execution.
 */

#include <Engine/Runtime/Context.hpp>
#include <Engine/Windowing/WindowSystem.hpp>

#include <cstdlib>
#include <variant>

/// @brief  Main entry point for the application.
/// @return EXIT_SUCCESS on successful execution, otherwise EXIT_FAILURE.
int main()
{
    Engine::Context engine;
    engine.initialize();

    Engine::WindowSystem windowSystem;
    auto windowLogger = engine.loggingSystem.createSubsystemLogger("WindowSystem");
    windowSystem.initialize(windowLogger);

    Engine::WindowIdentifier primaryWindow =
        windowSystem.createPrimaryWindow(Engine::WindowConfiguration{});

    bool isRunning = true;

    while (isRunning)
    {
        Engine::WindowEventPollResult windowEventPollResult = windowSystem.pollWindowEvents();

        if (windowEventPollResult.isApplicationQuitRequested)
        {
            isRunning = false;
        }

        for (Engine::WindowEvent const &windowEvent : windowEventPollResult.windowEvents)
        {
            Engine::WindowCloseRequested const *windowCloseRequested =
                std::get_if<Engine::WindowCloseRequested>(&windowEvent);
            bool shouldClosePrimaryWindow =
                windowCloseRequested != nullptr &&
                windowCloseRequested->windowIdentifier.value == primaryWindow.value;

            if (shouldClosePrimaryWindow)
            {
                isRunning = false;
            }
        }
    }

    windowSystem.shutdown();
    engine.loggingSystem.shutdown();

    return EXIT_SUCCESS;
}
