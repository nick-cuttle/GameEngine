/**
 * @file  main.cpp
 * @brief Entry point for the GameEngine.
 *
 * Handles initialization and main loop execution.
 */

#include <Engine/Runtime/Context.hpp>
#include <Engine/Windowing/WindowSystem.hpp>

#include <concepts>
#include <cstdlib>
#include <type_traits>
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
            std::visit(
                [&](auto const &windowEventPayload)
                {
                    using WindowEventPayload = std::decay_t<decltype(windowEventPayload)>;

                    if constexpr (std::same_as<WindowEventPayload, Engine::WindowCloseRequested>)
                    {
                        if (windowEventPayload.windowIdentifier == primaryWindow)
                        {
                            isRunning = false;
                        }
                    }
                },
                windowEvent);
        }
    }

    windowSystem.shutdown();
    engine.loggingSystem.shutdown();

    return EXIT_SUCCESS;
}
