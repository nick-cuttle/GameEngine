/**
 * @file  main.cpp
 * @brief Entry point for the GameEngine.
 *
 * Handles initialization and main loop execution.
 */

#include <Engine/Runtime/Context.hpp>
#include <Engine/Windowing/WindowSystem.hpp>

#include <cstdlib>

/// @brief  Main entry point for the application.
/// @return EXIT_SUCCESS on successful execution, otherwise EXIT_FAILURE.
int main()
{
    Engine::Context engine;
    engine.initialize();

    Engine::WindowSystem windowSystem;
    windowSystem.initialize();

    Engine::WindowIdentifier primaryWindow = windowSystem.createPrimaryWindow(
        "Game Engine", Engine::WindowSize{.width = 1280, .height = 720});

    bool isRunning = true;

    while (isRunning)
    {
        for (Engine::WindowEvent const &windowEvent : windowSystem.pollWindowEvents())
        {
            bool shouldClosePrimaryWindow =
                windowEvent.type == Engine::WindowEventType::CloseRequest &&
                windowEvent.windowIdentifier.value == primaryWindow.value;

            if (shouldClosePrimaryWindow)
            {
                isRunning = false;
            }
        }
    }

    windowSystem.shutdown();

    return EXIT_SUCCESS;
}
