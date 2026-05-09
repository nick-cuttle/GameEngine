/**
 * @file  main.cpp
 * @brief Entry point for the GameEngine.
 *
 * Handles initialization and main loop execution.
 */

#include <Engine/Rendering/Renderer.hpp>
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

    Engine::WindowConfiguration windowConfiguration;
    windowConfiguration.graphicsSurfaceCapability = Engine::GraphicsSurfaceCapability::OpenGL;

    Engine::WindowIdentifier primaryWindow = windowSystem.createPrimaryWindow(windowConfiguration);

    Engine::Renderer renderer;
    Engine::RendererConfiguration rendererConfiguration;
    rendererConfiguration.renderingBackendSelection = Engine::RenderingBackendSelection::OpenGL;
    rendererConfiguration.presentationMode = Engine::PresentationMode::VerticalSynchronization;

    auto rendererLogger = engine.loggingSystem.createSubsystemLogger("Renderer");
    renderer.initialize(rendererConfiguration, rendererLogger);
    renderer.attachGraphicsSurface(windowSystem, primaryWindow);

    Engine::LinearColor clearColor{.red = 0.02F, .green = 0.05F, .blue = 0.09F, .alpha = 1.0F};

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
            Engine::WindowCloseRequested const *windowCloseRequestedEvent =
                std::get_if<Engine::WindowCloseRequested>(&windowEvent);

            bool const isPrimaryWindowCloseRequested =
                windowCloseRequestedEvent != nullptr &&
                windowCloseRequestedEvent->windowIdentifier == primaryWindow;
            if (isPrimaryWindowCloseRequested)
            {
                isRunning = false;
            }
        }

        if (isRunning)
        {
            renderer.beginFrame();
            renderer.clear(clearColor);
            renderer.present();
        }
    }

    renderer.shutdown();
    windowSystem.shutdown();
    engine.loggingSystem.shutdown();

    return EXIT_SUCCESS;
}
