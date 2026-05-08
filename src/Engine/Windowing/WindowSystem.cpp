/**
 * @file  WindowSystem.cpp
 * @brief Implements SDL-backed engine window lifecycle and event polling.
 */

#include "WindowSystem.hpp"

#include <SDL3/SDL.h>
#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace
{

/// @brief Largest window dimension accepted before converting to the platform backend type.
constexpr std::uint32_t maximumPlatformWindowDimension =
    static_cast<std::uint32_t>(std::numeric_limits<int>::max());

/// @brief Maximum bytes reserved for SDL event descriptions used in trace logging.
constexpr int platformEventDescriptionBufferSize = 256;

/// @brief Formats an SDL event description for trace logging.
/// @param platformEvent SDL event to describe.
/// @return English description produced by SDL.
std::string describePlatformEvent(SDL_Event const &platformEvent)
{
    std::array<char, platformEventDescriptionBufferSize> eventDescriptionBuffer{};
    (void)SDL_GetEventDescription(&platformEvent, eventDescriptionBuffer.data(),
                                  static_cast<int>(eventDescriptionBuffer.size()));

    return eventDescriptionBuffer.data();
}

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

/// @brief Converts an SDL window event for a managed window into an engine window event.
/// @param platformEvent SDL event containing the window event payload to translate.
/// @param platformWindow SDL window associated with the event; required for display scale queries.
/// @return The translated engine event, or std::nullopt when the SDL event type is not handled.
std::optional<Engine::WindowEvent> translateManagedWindowEvent(SDL_Event const &platformEvent,
                                                               SDL_Window *platformWindow)
{
    Engine::WindowIdentifier windowIdentifier{
        static_cast<std::uint32_t>(platformEvent.window.windowID)};

    switch (platformEvent.type)
    {
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        return Engine::WindowCloseRequested{windowIdentifier};

    case SDL_EVENT_WINDOW_MOVED:
        return Engine::WindowMoved{
            windowIdentifier,
            Engine::WindowPosition{platformEvent.window.data1, platformEvent.window.data2}};

    case SDL_EVENT_WINDOW_RESIZED:
        return Engine::WindowSizeChanged{
            windowIdentifier,
            Engine::WindowSize{static_cast<std::uint32_t>(platformEvent.window.data1),
                               static_cast<std::uint32_t>(platformEvent.window.data2)}};

    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        return Engine::GraphicsSurfaceSizeChanged{
            windowIdentifier,
            Engine::GraphicsSurfaceSize{static_cast<std::uint32_t>(platformEvent.window.data1),
                                        static_cast<std::uint32_t>(platformEvent.window.data2)}};

    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        return Engine::WindowFocusGained{windowIdentifier};

    case SDL_EVENT_WINDOW_FOCUS_LOST:
        return Engine::WindowFocusLost{windowIdentifier};

    case SDL_EVENT_WINDOW_MINIMIZED:
        return Engine::WindowMinimized{windowIdentifier};

    case SDL_EVENT_WINDOW_RESTORED:
        return Engine::WindowRestored{windowIdentifier};

    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
        return Engine::WindowDisplayScaleChanged{windowIdentifier,
                                                 SDL_GetWindowDisplayScale(platformWindow)};
    default:
        return std::nullopt;
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
    releaseResources();
}

void WindowSystem::initialize(Logger logger)
{
    if (implementation->isInitialized)
    {
        m_Logger.trace("Implementation has already been initialized.");
        return;
    }
    m_Logger = logger;

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

    releaseResources();

    m_Logger.info("Window has been closed.");
}

WindowIdentifier WindowSystem::createPrimaryWindow(WindowConfiguration const &configuration)
{
    if (!implementation->isInitialized)
    {
        throw std::runtime_error("WindowSystem must be initialized before creating windows.");
    }

    if (configuration.size.width == 0 || configuration.size.height == 0)
    {
        throw std::invalid_argument("Window size must be greater than zero.");
    }

    if (configuration.size.width > maximumPlatformWindowDimension ||
        configuration.size.height > maximumPlatformWindowDimension)
    {
        throw std::invalid_argument("Window size exceeds the platform window dimension limit.");
    }

    int const platformWindowWidth = static_cast<int>(configuration.size.width);
    int const platformWindowHeight = static_cast<int>(configuration.size.height);

    SDL_WindowFlags windowFlags = configuration.isVisible ? 0 : SDL_WINDOW_HIDDEN;

    SDL_Window *window = SDL_CreateWindow(configuration.title.c_str(), platformWindowWidth,
                                          platformWindowHeight, windowFlags);

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
    static std::unordered_set<std::uint32_t> previouslyLoggedNonWindowEventTypes;

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

            m_Logger.trace("Application quit event has been requested.");
            continue;
        }

        if (event.type < SDL_EVENT_WINDOW_FIRST || event.type > SDL_EVENT_WINDOW_LAST)
        {
            std::uint32_t const eventType = static_cast<std::uint32_t>(event.type);

            if (previouslyLoggedNonWindowEventTypes.insert(eventType).second)
            {
                std::string const eventDescription = describePlatformEvent(event);

                m_Logger.trace("Non-window event captured: {} ({})", eventDescription,
                               eventType);
            }

            continue;
        }

        auto windowIterator = implementation->windowByIdentifier.find(
            static_cast<std::uint32_t>(event.window.windowID));

        if (windowIterator == implementation->windowByIdentifier.end())
        {
            m_Logger.trace("Ignoring window event for unmanaged window identifier {}.",
                           event.window.windowID);
            continue;
        }

        std::optional<WindowEvent> windowEvent =
            translateManagedWindowEvent(event, windowIterator->second);

        if (windowEvent.has_value())
        {
            pollResult.windowEvents.push_back(*windowEvent);
        }
    }

    return pollResult;
}

void WindowSystem::releaseResources() noexcept
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

} // namespace Engine
