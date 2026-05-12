/**
 * @file  OpenGLRenderingBackend.cpp
 * @brief Implements the OpenGL Rendering Backend and GLAD loading path.
 */

#include "OpenGLRenderingBackend.hpp"

#include <Engine/Rendering/Internal/GraphicsSurfaceFactory.hpp>

#include <SDL3/SDL.h>
#include <glad/gl.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
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
    /// @brief Platform graphics surface currently attached to this backend.
    PlatformGraphicsSurface graphicsSurface;
    /// @brief OpenGL context owned by this backend.
    SDL_GLContext renderingContext{nullptr};
    /// @brief Engine window associated with the attached graphics surface.
    WindowIdentifier windowIdentifier{};
    /// @brief Whether the backend has an attached graphics surface and context.
    bool isGraphicsSurfaceAttached{false};
    /// @brief Whether the current frame has a drawable graphics surface.
    bool isCurrentFrameDrawable{false};
    /// @brief Pending graphics surface size reported by Window Events for the next frame boundary.
    std::optional<GraphicsSurfaceSize> pendingGraphicsSurfaceSize;

    /// @brief Makes the backend OpenGL context current for the attached graphics surface.
    /// @throws std::runtime_error when SDL cannot make the context current.
    void makeContextCurrent()
    {
        if (!SDL_GL_MakeCurrent(graphicsSurface.platformWindow, renderingContext))
        {
            throw platformError("Failed to make OpenGL context current");
        }
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
    if (implementation->isGraphicsSurfaceAttached)
    {
        throw std::runtime_error("Renderer already has an attached Graphics Surface.");
    }

    implementation->graphicsSurface =
        GraphicsSurfaceFactory::createOpenGLGraphicsSurface(windowSystem, windowIdentifier);
    implementation->windowIdentifier = windowIdentifier;
    implementation->renderingContext =
        SDL_GL_CreateContext(implementation->graphicsSurface.platformWindow);

    if (implementation->renderingContext == nullptr)
    {
        throw platformError("Failed to create OpenGL context");
    }

    implementation->isGraphicsSurfaceAttached = true;

    try
    {
        implementation->makeContextCurrent();
    }
    catch (...)
    {
        shutdown();
        throw;
    }

    if (gladLoadGL(loadOpenGLProcedure) == 0)
    {
        shutdown();
        throw std::runtime_error("Failed to load OpenGL procedures with GLAD.");
    }

    if (GLAD_GL_VERSION_4_6 == 0)
    {
        shutdown();
        throw std::runtime_error("OpenGL 4.6 core profile is required but not available.");
    }

    if (!SDL_GL_SetSwapInterval(swapIntervalForPresentationMode(presentationMode)))
    {
        shutdown();
        throw platformError("Failed to set OpenGL Presentation Mode");
    }

    // Initializes context, surface size, and viewport; drawability is handled by regular frame calls.
    (void)beginFrame();

    logger.info("OpenGL 4.6 core Rendering Backend has been initialized.");
}

void OpenGLRenderingBackend::handleWindowEvent(WindowEvent const &windowEvent)
{
    if (!implementation->isGraphicsSurfaceAttached)
    {
        return;
    }

    GraphicsSurfaceSizeChanged const *graphicsSurfaceSizeChanged{std::get_if<GraphicsSurfaceSizeChanged>(&windowEvent)};
    if (graphicsSurfaceSizeChanged == nullptr ||
        graphicsSurfaceSizeChanged->windowIdentifier != implementation->windowIdentifier)
    {
        return;
    }

    implementation->pendingGraphicsSurfaceSize = graphicsSurfaceSizeChanged->graphicsSurfaceSize;
}

bool OpenGLRenderingBackend::beginFrame()
{
    if (!implementation->isGraphicsSurfaceAttached)
    {
        throw std::runtime_error("Renderer must have an attached Graphics Surface before drawing.");
    }

    implementation->makeContextCurrent();

    // Update the engine state with the pending graphics surface size, if available.
    if (implementation->pendingGraphicsSurfaceSize.has_value())
    {
        implementation->graphicsSurface.graphicsSurfaceSize =
            *implementation->pendingGraphicsSurfaceSize;
        implementation->pendingGraphicsSurfaceSize.reset();
    }
    else
    {
        implementation->graphicsSurface.graphicsSurfaceSize =
            queryGraphicsSurfaceSize(implementation->graphicsSurface.platformWindow);
    }

    implementation->isCurrentFrameDrawable =
        implementation->graphicsSurface.graphicsSurfaceSize.width > 0 &&
        implementation->graphicsSurface.graphicsSurfaceSize.height > 0;

    if (!implementation->isCurrentFrameDrawable)
    {
        return false;
    }

    glViewport(0, 0,
               static_cast<GLsizei>(implementation->graphicsSurface.graphicsSurfaceSize.width),
               static_cast<GLsizei>(implementation->graphicsSurface.graphicsSurfaceSize.height));

    return true;
}

void OpenGLRenderingBackend::clear(LinearColor const &color)
{
    if (!implementation->isGraphicsSurfaceAttached)
    {
        throw std::runtime_error("Renderer must have an attached Graphics Surface before drawing.");
    }

    if (!implementation->isCurrentFrameDrawable)
    {
        return;
    }

    implementation->makeContextCurrent();

    glClearColor(color.red, color.green, color.blue, color.alpha);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLRenderingBackend::present()
{
    if (!implementation->isGraphicsSurfaceAttached)
    {
        throw std::runtime_error("Renderer must have an attached Graphics Surface before drawing.");
    }

    if (!implementation->isCurrentFrameDrawable)
    {
        return;
    }

    implementation->makeContextCurrent();

    if (!SDL_GL_SwapWindow(implementation->graphicsSurface.platformWindow))
    {
        throw platformError("Failed to present OpenGL frame");
    }
}

void OpenGLRenderingBackend::shutdown() noexcept
{
    if (implementation->renderingContext != nullptr)
    {
        if (implementation->graphicsSurface.platformWindow != nullptr)
        {
            (void)SDL_GL_MakeCurrent(implementation->graphicsSurface.platformWindow, nullptr);
        }

        (void)SDL_GL_DestroyContext(implementation->renderingContext);
    }

    implementation->renderingContext = nullptr;
    implementation->graphicsSurface = {};
    implementation->windowIdentifier = {};
    implementation->isGraphicsSurfaceAttached = false;
    implementation->isCurrentFrameDrawable = false;
    implementation->pendingGraphicsSurfaceSize.reset();
}

} // namespace Engine::Rendering::Internal
