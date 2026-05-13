/**
 * @file  OpenGLRenderingBackend.cpp
 * @brief Implements the OpenGL Rendering Backend and GLAD loading path.
 */

#include "OpenGLRenderingBackend.hpp"
#include "Engine/Windowing/WindowSystem.hpp"

#include <Engine/Rendering/Internal/GraphicsSurfaceFactory.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <glad/gl.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace
{

/// @brief Converts a presentation mode to the SDL swap interval value.
/// @param presentationMode Engine-owned presentation mode.
/// @return SDL swap interval value matching the requested presentation timing.
int swapIntervalForPresentationMode(Engine::PresentationMode presentationMode)
{
    switch (presentationMode)
    {
    case Engine::PresentationMode::Immediate:
        return 0;
    case Engine::PresentationMode::VerticalSynchronization:
        return 1;
    }

    throw std::runtime_error("Unsupported Presentation Mode.");
}

/// @brief Converts an SDL error into a runtime exception with operation context.
/// @param operationDescription Description of the failed SDL operation.
/// @return Runtime exception containing the SDL error text.
std::runtime_error platformError(std::string_view operationDescription)
{
    return std::runtime_error(std::string(operationDescription) + ": " + SDL_GetError());
}

/// @brief Loads one OpenGL procedure through SDL for GLAD.
/// @param procedureName OpenGL procedure name requested by GLAD.
/// @return Loaded procedure address, or null when SDL cannot resolve it.
GLADapiproc loadOpenGLProcedure(char const *procedureName)
{
    return reinterpret_cast<GLADapiproc>(SDL_GL_GetProcAddress(procedureName));
}

/// @brief Queries the drawable graphics surface size for a platform window.
/// @param platformWindow SDL window that owns the graphics surface.
/// @return Current drawable graphics surface size in pixels.
/// @throws std::runtime_error when SDL cannot report the size.
Engine::GraphicsSurfaceSize queryGraphicsSurfaceSize(SDL_Window *platformWindow)
{
    int graphicsSurfaceWidth = 0;
    int graphicsSurfaceHeight = 0;

    if (!SDL_GetWindowSizeInPixels(platformWindow, &graphicsSurfaceWidth, &graphicsSurfaceHeight))
    {
        throw platformError("Failed to query OpenGL Graphics Surface size");
    }

    return Engine::GraphicsSurfaceSize{
        static_cast<std::uint32_t>(std::max(graphicsSurfaceWidth, 0)),
        static_cast<std::uint32_t>(std::max(graphicsSurfaceHeight, 0))};
}

} // namespace

namespace Engine::Rendering::Internal
{

struct OpenGLRenderingBackend::Implementation
{
    /// @brief OpenGL state owned for one renderer-managed Graphics Surface.
    struct AttachedGraphicsSurface
    {
        /// @brief Platform graphics surface currently attached to this backend.
        PlatformGraphicsSurface graphicsSurface;
        /// @brief OpenGL context owned by this backend surface.
        SDL_GLContext renderingContext{nullptr};
        /// @brief Pending graphics surface size reported by Window Events for the next frame.
        std::optional<GraphicsSurfaceSize> pendingGraphicsSurfaceSize;
    };

    /// @brief Renderer-managed Graphics Surfaces keyed by Window Identifier.
    std::unordered_map<std::uint32_t, AttachedGraphicsSurface> graphicsSurfaceByWindowIdentifier;
    /// @brief Active surface used by legacy single-surface frame commands.
    WindowIdentifier activeWindowIdentifier{};
    /// @brief Whether the current frame has a drawable graphics surface.
    bool isCurrentFrameDrawable{false};

    /// @brief Makes the backend OpenGL context current for the attached graphics surface.
    /// @param graphicsSurface Surface whose context is made current.
    /// @throws std::runtime_error when SDL cannot make the context current.
    void makeContextCurrent(AttachedGraphicsSurface const &graphicsSurface)
    {
        if (!SDL_GL_MakeCurrent(graphicsSurface.graphicsSurface.platformWindow,
                                graphicsSurface.renderingContext))
        {
            throw platformError("Failed to make OpenGL context current");
        }
    }

    /// @brief Returns the active Graphics Surface for frame commands.
    /// @return Active surface state.
    /// @throws std::runtime_error when no Graphics Surface is attached.
    AttachedGraphicsSurface &activeGraphicsSurface()
    {
        auto graphicsSurfaceIterator =
            graphicsSurfaceByWindowIdentifier.find(activeWindowIdentifier.value);

        if (graphicsSurfaceIterator == graphicsSurfaceByWindowIdentifier.end())
        {
            throw std::runtime_error(
                "Renderer must have an attached Graphics Surface before drawing.");
        }

        return graphicsSurfaceIterator->second;
    }

    /// @brief Releases the OpenGL context and graphics surface of an attached Graphics Surface.
    /// @param attachedGraphicsSurface Surface to release.
    void releaseAttachedGraphicsSurface(AttachedGraphicsSurface &attachedGraphicsSurface) noexcept
    {
        if (attachedGraphicsSurface.renderingContext != nullptr)
        {
            if (attachedGraphicsSurface.graphicsSurface.platformWindow != nullptr)
            {
                (void)SDL_GL_MakeCurrent(attachedGraphicsSurface.graphicsSurface.platformWindow,
                                         nullptr);
            }
            (void)SDL_GL_DestroyContext(attachedGraphicsSurface.renderingContext);
            attachedGraphicsSurface.renderingContext = nullptr;
        }

        GraphicsSurfaceFactory::releaseGraphicsSurface(attachedGraphicsSurface.graphicsSurface);
    }
};

OpenGLRenderingBackend::OpenGLRenderingBackend(Logger logger, PresentationMode presentationMode)
    : implementation(std::make_unique<Implementation>()), logger(logger),
      presentationMode(presentationMode)
{
}

OpenGLRenderingBackend::~OpenGLRenderingBackend()
{
    shutdown();
}

void OpenGLRenderingBackend::attachGraphicsSurface(WindowSystem &windowSystem,
                                                   WindowIdentifier windowIdentifier)
{
    if (implementation->graphicsSurfaceByWindowIdentifier.contains(windowIdentifier.value))
    {
        throw std::runtime_error(
            "Renderer already has a Graphics Surface attached for this Window Identifier.");
    }

    Implementation::AttachedGraphicsSurface attachedGraphicsSurface;
    attachedGraphicsSurface.graphicsSurface =
        GraphicsSurfaceFactory::createOpenGLGraphicsSurface(windowSystem, windowIdentifier);

    if (!implementation->graphicsSurfaceByWindowIdentifier.empty())
    {
        Implementation::AttachedGraphicsSurface const &sharedGraphicsSurface =
            implementation->graphicsSurfaceByWindowIdentifier.begin()->second;
        implementation->makeContextCurrent(sharedGraphicsSurface);

        if (!SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1))
        {
            implementation->releaseAttachedGraphicsSurface(attachedGraphicsSurface);
            throw platformError("Failed to enable OpenGL context resource sharing");
        }
    }

    attachedGraphicsSurface.renderingContext =
        SDL_GL_CreateContext(attachedGraphicsSurface.graphicsSurface.platformWindow);

    if (attachedGraphicsSurface.renderingContext == nullptr)
    {
        implementation->releaseAttachedGraphicsSurface(attachedGraphicsSurface);
        throw platformError("Failed to create OpenGL context");
    }

    try
    {
        implementation->makeContextCurrent(attachedGraphicsSurface);
    }
    catch (...)
    {
        implementation->releaseAttachedGraphicsSurface(attachedGraphicsSurface);
        throw;
    }

    if (gladLoadGL(loadOpenGLProcedure) == 0)
    {
        implementation->releaseAttachedGraphicsSurface(attachedGraphicsSurface);
        throw std::runtime_error("Failed to load OpenGL procedures with GLAD.");
    }

    if (GLAD_GL_VERSION_4_6 == 0)
    {
        implementation->releaseAttachedGraphicsSurface(attachedGraphicsSurface);
        throw std::runtime_error("OpenGL 4.6 core profile is required but not available.");
    }

    if (!SDL_GL_SetSwapInterval(swapIntervalForPresentationMode(presentationMode)))
    {
        implementation->releaseAttachedGraphicsSurface(attachedGraphicsSurface);
        throw platformError("Failed to set OpenGL Presentation Mode");
    }

    implementation->activeWindowIdentifier = windowIdentifier;
    implementation->graphicsSurfaceByWindowIdentifier.emplace(windowIdentifier.value,
                                                              std::move(attachedGraphicsSurface));

    // Initializes context, surface size, and viewport; drawability is handled by regular frame
    // calls.
    (void)beginFrame();

    logger.info("OpenGL 4.6 core Rendering Backend has been initialized.");
}

void OpenGLRenderingBackend::detachGraphicsSurface(WindowIdentifier windowIdentifier) noexcept
{
    auto graphicsSurfaceIterator =
        implementation->graphicsSurfaceByWindowIdentifier.find(windowIdentifier.value);

    if (graphicsSurfaceIterator == implementation->graphicsSurfaceByWindowIdentifier.end())
    {
        return;
    }

    Implementation::AttachedGraphicsSurface &attachedGraphicsSurface =
        graphicsSurfaceIterator->second;

    implementation->releaseAttachedGraphicsSurface(attachedGraphicsSurface);
    implementation->graphicsSurfaceByWindowIdentifier.erase(graphicsSurfaceIterator);

    if (implementation->activeWindowIdentifier == windowIdentifier)
    {
        implementation->activeWindowIdentifier =
            implementation->graphicsSurfaceByWindowIdentifier.empty()
                ? WindowIdentifier{}
                : WindowIdentifier{
                      implementation->graphicsSurfaceByWindowIdentifier.begin()->first};
    }

    implementation->isCurrentFrameDrawable = false;
}

void OpenGLRenderingBackend::handleWindowEvent(WindowEvent const &windowEvent)
{
    GraphicsSurfaceSizeChanged const *graphicsSurfaceSizeChanged{
        std::get_if<GraphicsSurfaceSizeChanged>(&windowEvent)};
    if (graphicsSurfaceSizeChanged == nullptr)
    {
        return;
    }

    auto graphicsSurfaceIterator = implementation->graphicsSurfaceByWindowIdentifier.find(
        graphicsSurfaceSizeChanged->windowIdentifier.value);

    if (graphicsSurfaceIterator == implementation->graphicsSurfaceByWindowIdentifier.end())
    {
        return;
    }

    graphicsSurfaceIterator->second.pendingGraphicsSurfaceSize =
        graphicsSurfaceSizeChanged->graphicsSurfaceSize;
}

bool OpenGLRenderingBackend::beginFrame()
{
    Implementation::AttachedGraphicsSurface &activeGraphicsSurface =
        implementation->activeGraphicsSurface();

    implementation->makeContextCurrent(activeGraphicsSurface);

    // Update the engine state with the pending graphics surface size, if available.
    if (activeGraphicsSurface.pendingGraphicsSurfaceSize.has_value())
    {
        activeGraphicsSurface.graphicsSurface.graphicsSurfaceSize =
            *activeGraphicsSurface.pendingGraphicsSurfaceSize;
        activeGraphicsSurface.pendingGraphicsSurfaceSize.reset();
    }
    else
    {
        activeGraphicsSurface.graphicsSurface.graphicsSurfaceSize =
            queryGraphicsSurfaceSize(activeGraphicsSurface.graphicsSurface.platformWindow);
    }

    implementation->isCurrentFrameDrawable =
        activeGraphicsSurface.graphicsSurface.graphicsSurfaceSize.width > 0 &&
        activeGraphicsSurface.graphicsSurface.graphicsSurfaceSize.height > 0;

    if (!implementation->isCurrentFrameDrawable)
    {
        return false;
    }

    glViewport(
        0, 0, static_cast<GLsizei>(activeGraphicsSurface.graphicsSurface.graphicsSurfaceSize.width),
        static_cast<GLsizei>(activeGraphicsSurface.graphicsSurface.graphicsSurfaceSize.height));

    return true;
}

void OpenGLRenderingBackend::clear(LinearColor const &color)
{
    Implementation::AttachedGraphicsSurface &activeGraphicsSurface =
        implementation->activeGraphicsSurface();

    if (!implementation->isCurrentFrameDrawable)
    {
        return;
    }

    implementation->makeContextCurrent(activeGraphicsSurface);

    glClearColor(color.red, color.green, color.blue, color.alpha);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLRenderingBackend::present()
{
    Implementation::AttachedGraphicsSurface &activeGraphicsSurface =
        implementation->activeGraphicsSurface();

    if (!implementation->isCurrentFrameDrawable)
    {
        return;
    }

    implementation->makeContextCurrent(activeGraphicsSurface);

    if (!SDL_GL_SwapWindow(activeGraphicsSurface.graphicsSurface.platformWindow))
    {
        throw platformError("Failed to present OpenGL frame");
    }
}

void OpenGLRenderingBackend::shutdown() noexcept
{
    for (auto &[windowIdentifier, attachedGraphicsSurface] :
         implementation->graphicsSurfaceByWindowIdentifier)
    {
        implementation->releaseAttachedGraphicsSurface(attachedGraphicsSurface);
    }

    implementation->graphicsSurfaceByWindowIdentifier.clear();
    implementation->activeWindowIdentifier = {};
    implementation->isCurrentFrameDrawable = false;
}

} // namespace Engine::Rendering::Internal
