/**
 * @file  WindowSystem.cpp
 * @brief Implements SDL-backed engine window lifecycle and event polling.
 */

#include "WindowSystem.hpp"

#include <SDL3/SDL.h>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace
{

/// @brief Commits an initial window surface so desktop compositors can map the window.
/// @param window Platform window that receives the initial visibility buffer.
/// @throws std::runtime_error if SDL cannot acquire, fill, or present the window surface.
void presentInitialVisibilityBuffer(SDL_Window *window)
{
    SDL_Surface *windowSurface = SDL_GetWindowSurface(window);

    if (windowSurface == nullptr)
    {
        throw std::runtime_error(SDL_GetError());
    }

    // The initial color is temporary. The renderer will replace it once the frame loop starts.
    Uint32 backgroundColor = SDL_MapSurfaceRGBA(windowSurface, 255, 0, 0, 255);

    if (!SDL_FillSurfaceRect(windowSurface, nullptr, backgroundColor))
    {
        throw std::runtime_error(SDL_GetError());
    }

    // Wayland does not map a new window until the application commits a first buffer.
    if (!SDL_UpdateWindowSurface(window))
    {
        throw std::runtime_error(SDL_GetError());
    }
}

} // namespace

namespace Engine
{

struct WindowSystem::Implementation
{
    /// @brief Whether this instance currently owns an initialized SDL video subsystem.
    bool isInitialized = false;
    /// @brief Managed SDL windows keyed by the engine-visible SDL window identifier.
    std::unordered_map<std::uint32_t, SDL_Window *> windowByIdentifier;
};

WindowSystem::WindowSystem() : implementation(std::make_unique<Implementation>())
{
}

WindowSystem::~WindowSystem()
{
    if (implementation->isInitialized)
    {
        shutdown();
    }
}

void WindowSystem::initialize(Logger logger)
{
    m_Logger = logger;

    if (implementation->isInitialized)
    {
        m_Logger.trace("Implementation has already been initialized.");
        return;
    }

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        throw std::runtime_error(SDL_GetError());
    }

    implementation->isInitialized = true;
    m_Logger.info("Window has been created.");
}

void WindowSystem::shutdown()
{
    if (!implementation->isInitialized)
    {
        m_Logger.trace("Cannot shutdown if not initialized.");
        return;
    }

    for (auto const &[windowIdentifier, window] : implementation->windowByIdentifier)
    {
        SDL_DestroyWindow(window);
    }

    implementation->windowByIdentifier.clear();

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    implementation->isInitialized = false;

    m_Logger.info("Window has been closed.");
}

WindowIdentifier WindowSystem::createPrimaryWindow(WindowConfiguration const &configuration)
{
    if (!implementation->isInitialized)
    {
        throw std::runtime_error("WindowSystem must be initialized before creating windows.");
    }

    if (configuration.size.width <= 0 || configuration.size.height <= 0)
    {
        throw std::invalid_argument("Window size must be greater than zero.");
    }

    SDL_WindowFlags windowFlags = configuration.isVisible ? 0 : SDL_WINDOW_HIDDEN;

    SDL_Window *window = SDL_CreateWindow(configuration.title.c_str(), configuration.size.width,
                                          configuration.size.height, windowFlags);

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

    WindowIdentifier returnedWindowIdentifier{static_cast<std::uint32_t>(windowIdentifier)};

    if (configuration.isVisible)
    {
        try
        {
            // Some platforms require a first committed buffer before a newly visible window maps.
            presentInitialVisibilityBuffer(window);
        }
        catch (std::exception const &exception)
        {
            SDL_DestroyWindow(window);

            throw exception;
        }
    }

    implementation->windowByIdentifier.emplace(returnedWindowIdentifier.value, window);

    return returnedWindowIdentifier;
}

WindowEventPollResult WindowSystem::pollWindowEvents()
{
    WindowEventPollResult pollResult;

    if (!implementation->isInitialized)
    {
        return pollResult;
    }

    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            pollResult.isApplicationQuitRequested = true;
            continue;
        }

        if (event.type != SDL_EVENT_WINDOW_CLOSE_REQUESTED)
        {
            continue;
        }

        std::uint32_t windowIdentifier = static_cast<std::uint32_t>(event.window.windowID);

        if (!implementation->windowByIdentifier.contains(windowIdentifier))
        {
            // SDL can report close events for windows not owned by this Window System instance.
            continue;
        }

        pollResult.windowEvents.push_back(WindowEvent{
            .type = WindowEventType::CloseRequest,
            .windowIdentifier = WindowIdentifier{windowIdentifier},
        });
    }

    return pollResult;
}

} // namespace Engine
