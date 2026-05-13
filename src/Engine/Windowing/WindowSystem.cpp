/**
 * @file  WindowSystem.cpp
 * @brief Implements SDL-backed engine window lifecycle and event polling.
 */

#include "WindowSystem.hpp"

#include <Engine/Rendering/Internal/GraphicsSurfaceFactory.hpp>

#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
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
/// @brief OpenGL major version requested for engine renderer windows.
constexpr int openGLMajorVersion = 4;
/// @brief OpenGL minor version requested for engine renderer windows.
constexpr int openGLMinorVersion = 6;

/// @brief Converts an SDL error into a runtime exception with operation context.
/// @param operationDescription Description of the failed SDL operation.
/// @return Runtime exception containing the SDL error text.
std::runtime_error platformError(char const *operationDescription)
{
    return std::runtime_error(std::string(operationDescription) + ": " + SDL_GetError());
}

/// @brief Sets an OpenGL window attribute before SDL chooses the window visual.
/// @param attribute SDL OpenGL attribute to set.
/// @param value Value assigned to the attribute.
/// @param operationDescription Description used if SDL rejects the attribute.
void setOpenGLWindowAttribute(SDL_GLAttr attribute, int value, char const *operationDescription)
{
    if (!SDL_GL_SetAttribute(attribute, value))
    {
        throw platformError(operationDescription);
    }
}

/// @brief Configures the OpenGL context attributes required by renderer-capable windows.
/// @throws std::runtime_error if SDL rejects an OpenGL attribute.
void configureOpenGLWindowAttributes()
{
    setOpenGLWindowAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, openGLMajorVersion,
                             "Failed to set OpenGL major version");
    setOpenGLWindowAttribute(SDL_GL_CONTEXT_MINOR_VERSION, openGLMinorVersion,
                             "Failed to set OpenGL minor version");
    setOpenGLWindowAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE,
                             "Failed to set OpenGL core profile");
    setOpenGLWindowAttribute(SDL_GL_DOUBLEBUFFER, 1, "Failed to enable OpenGL double buffering");
}

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
    /// @brief Renderer-owned Graphics Surface attachment counts keyed by Window Identifier.
    std::unordered_map<std::uint32_t, std::uint32_t> attachedSurfaceCountByIdentifier;
    /// @brief Current Primary Window identifier, or zero when none exists.
    WindowIdentifier primaryWindowIdentifier{};
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
    if (implementation->primaryWindowIdentifier.value != 0)
    {
        throw std::runtime_error("Primary window has already been created.");
    }

    WindowIdentifier windowIdentifier = createWindow(configuration);
    implementation->primaryWindowIdentifier = windowIdentifier;

    return windowIdentifier;
}

WindowIdentifier WindowSystem::createWindow(WindowConfiguration const &configuration)
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

    SDL_WindowFlags windowFlags = 0;

    if (!configuration.isVisible)
    {
        // Hide the window initially; will be shown when the first frame is drawn.
        windowFlags |= SDL_WINDOW_HIDDEN;
    }

    if (configuration.isResizable)
    {
        // Allow the window to be resized interactively by the desktop environment.
        windowFlags |= SDL_WINDOW_RESIZABLE;
    }

    if (configuration.graphicsSurfaceCapability == GraphicsSurfaceCapability::OpenGL)
    {
        configureOpenGLWindowAttributes();
        windowFlags |= SDL_WINDOW_OPENGL;
    }

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

    bool const shouldPresentInitialVisibilityBuffer =
        configuration.isVisible &&
        configuration.graphicsSurfaceCapability == GraphicsSurfaceCapability::None;

    if (shouldPresentInitialVisibilityBuffer)
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

void WindowSystem::destroyWindow(WindowIdentifier windowIdentifier)
{
    auto windowIterator = implementation->windowByIdentifier.find(windowIdentifier.value);

    if (windowIterator == implementation->windowByIdentifier.end())
    {
        throw std::invalid_argument("Cannot destroy an unknown Window Identifier.");
    }

    auto attachedSurfaceCountIterator =
        implementation->attachedSurfaceCountByIdentifier.find(windowIdentifier.value);

    if (attachedSurfaceCountIterator != implementation->attachedSurfaceCountByIdentifier.end() &&
        attachedSurfaceCountIterator->second > 0)
    {
        throw std::runtime_error(
            "Cannot destroy a window while a Graphics Surface is still attached.");
    }

    SDL_DestroyWindow(windowIterator->second);
    implementation->windowByIdentifier.erase(windowIterator);
    implementation->attachedSurfaceCountByIdentifier.erase(windowIdentifier.value);

    if (implementation->primaryWindowIdentifier == windowIdentifier)
    {
        implementation->primaryWindowIdentifier = {};
    }
}

bool WindowSystem::handleDefaultCloseRequest(WindowIdentifier windowIdentifier)
{
    if (windowIdentifier == implementation->primaryWindowIdentifier)
    {
        return true;
    }

    destroyWindow(windowIdentifier);
    return false;
}

bool WindowSystem::isWindowManaged(WindowIdentifier windowIdentifier) const
{
    return implementation->windowByIdentifier.contains(windowIdentifier.value);
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

                m_Logger.trace("Non-window event captured: {} ({})", eventDescription, eventType);
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

void WindowSystem::registerAttachedGraphicsSurface(WindowIdentifier windowIdentifier)
{
    if (!isWindowManaged(windowIdentifier))
    {
        throw std::invalid_argument(
            "Cannot attach a Graphics Surface for an unknown Window Identifier.");
    }

    ++implementation->attachedSurfaceCountByIdentifier[windowIdentifier.value];
}

void WindowSystem::unregisterAttachedGraphicsSurface(WindowIdentifier windowIdentifier) noexcept
{
    auto attachedSurfaceCountIterator =
        implementation->attachedSurfaceCountByIdentifier.find(windowIdentifier.value);

    if (attachedSurfaceCountIterator == implementation->attachedSurfaceCountByIdentifier.end())
    {
        return;
    }

    // Detach the Graphics Surface from the window if it is the last attached surface.
    if (attachedSurfaceCountIterator->second > 1)
    {
        --attachedSurfaceCountIterator->second;
        return;
    }

    implementation->attachedSurfaceCountByIdentifier.erase(attachedSurfaceCountIterator);
}

namespace Rendering::Internal
{

PlatformGraphicsSurface
GraphicsSurfaceFactory::createOpenGLGraphicsSurface(WindowSystem &windowSystem,
                                                    WindowIdentifier windowIdentifier)
{
    if (!windowSystem.implementation->isInitialized)
    {
        throw std::runtime_error(
            "WindowSystem must be initialized before creating Graphics Surfaces.");
    }

    auto windowIterator =
        windowSystem.implementation->windowByIdentifier.find(windowIdentifier.value);

    if (windowIterator == windowSystem.implementation->windowByIdentifier.end())
    {
        throw std::invalid_argument(
            "Cannot create a Graphics Surface for an unknown Window Identifier.");
    }

    int graphicsSurfaceWidth = 0;
    int graphicsSurfaceHeight = 0;

    if (!SDL_GetWindowSizeInPixels(windowIterator->second, &graphicsSurfaceWidth,
                                   &graphicsSurfaceHeight))
    {
        throw std::runtime_error(SDL_GetError());
    }

    windowSystem.registerAttachedGraphicsSurface(windowIdentifier);

    return PlatformGraphicsSurface{
        &windowSystem, windowIdentifier, windowIterator->second,
        GraphicsSurfaceSize{static_cast<std::uint32_t>(std::max(graphicsSurfaceWidth, 0)),
                            static_cast<std::uint32_t>(std::max(graphicsSurfaceHeight, 0))}};
}

void GraphicsSurfaceFactory::releaseGraphicsSurface(
    PlatformGraphicsSurface &graphicsSurface) noexcept
{
    if (graphicsSurface.windowSystem != nullptr)
    {
        graphicsSurface.windowSystem->unregisterAttachedGraphicsSurface(
            graphicsSurface.windowIdentifier);
    }

    graphicsSurface = {};
}

} // namespace Rendering::Internal

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
    implementation->attachedSurfaceCountByIdentifier.clear();
    implementation->primaryWindowIdentifier = {};

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    implementation->isInitialized = false;
}

} // namespace Engine
