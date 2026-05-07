#include "WindowSystem.hpp"
#include <SDL3/SDL.h>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace Engine
{

struct WindowSystem::Implementation
{
    bool isInitialized = false;
    std::unordered_map<unsigned int, SDL_Window *> windowByIdentifier;
};

WindowSystem::WindowSystem() : implementation(std::make_unique<Implementation>())
{
}

WindowSystem::~WindowSystem()
{
    shutdown();
}

void WindowSystem::initialize()
{
    if (implementation->isInitialized)
    {
        return;
    }

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        throw std::runtime_error(SDL_GetError());
    }

    implementation->isInitialized = true;
}

void WindowSystem::shutdown()
{
    if (!implementation->isInitialized)
    {
        return;
    }

    for (auto const &[windowIdentifier, window] : implementation->windowByIdentifier)
    {
        SDL_DestroyWindow(window);
    }

    implementation->windowByIdentifier.clear();

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    implementation->isInitialized = false;
}

WindowIdentifier WindowSystem::createPrimaryWindow(std::string_view title, WindowSize size)
{
    if (!implementation->isInitialized)
    {
        throw std::runtime_error("WindowSystem must be initialized before creating windows.");
    }

    if (size.width <= 0 || size.height <= 0)
    {
        throw std::invalid_argument("Window size must be greater than zero.");
    }

    std::string titleString{title};

    SDL_Window *window = SDL_CreateWindow(titleString.c_str(), size.width, size.height, 0);

    if (window == nullptr)
    {
        throw std::runtime_error(SDL_GetError());
    }

    SDL_WindowID windowIdentifier = SDL_GetWindowID(window);

    if (windowIdentifier == 0)
    {
        SDL_DestroyWindow(window);

        throw std::runtime_error(SDL_GetError());
    }

    WindowIdentifier returnedWindowIdentifier{static_cast<unsigned int>(windowIdentifier)};

    implementation->windowByIdentifier.emplace(returnedWindowIdentifier.value, window);

    return returnedWindowIdentifier;
}

std::vector<WindowEvent> WindowSystem::pollWindowEvents()
{
    std::vector<WindowEvent> windowEvents;

    if (!implementation->isInitialized)
    {
        return windowEvents;
    }

    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type != SDL_EVENT_WINDOW_CLOSE_REQUESTED)
        {
            continue;
        }

        unsigned int windowIdentifier = static_cast<unsigned int>(event.window.windowID);

        if (!implementation->windowByIdentifier.contains(windowIdentifier))
        {
            continue;
        }

        windowEvents.push_back(WindowEvent{
            .type = WindowEventType::CloseRequest,
            .windowIdentifier = WindowIdentifier{windowIdentifier},
        });

        return windowEvents;
    }

    return windowEvents;
}

} // namespace Engine
