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
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{

/// @brief OpenGL major version requested for the first renderer path.
constexpr int openGLMajorVersion = 4;
/// @brief OpenGL minor version requested for the first renderer path.
constexpr int openGLMinorVersion = 6;

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

/// @brief Sets an OpenGL context attribute before creating the context.
/// @param attribute SDL OpenGL attribute to set.
/// @param value Value assigned to the attribute.
/// @param operationDescription Description used if SDL rejects the attribute.
void setOpenGLAttribute(SDL_GLAttr attribute, int value, std::string_view operationDescription)
{
    if (!SDL_GL_SetAttribute(attribute, value))
    {
        throw platformError(operationDescription);
    }
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
    SDL_GLContext renderingContext = nullptr;
    /// @brief Whether the backend has an attached graphics surface and context.
    bool isGraphicsSurfaceAttached = false;
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

    setOpenGLAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, openGLMajorVersion,
                       "Failed to set OpenGL major version");
    setOpenGLAttribute(SDL_GL_CONTEXT_MINOR_VERSION, openGLMinorVersion,
                       "Failed to set OpenGL minor version");
    setOpenGLAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE,
                       "Failed to set OpenGL core profile");
    setOpenGLAttribute(SDL_GL_DOUBLEBUFFER, 1, "Failed to enable OpenGL double buffering");

    implementation->graphicsSurface =
        GraphicsSurfaceFactory::createOpenGLGraphicsSurface(windowSystem, windowIdentifier);
    implementation->renderingContext =
        SDL_GL_CreateContext(implementation->graphicsSurface.platformWindow);

    if (implementation->renderingContext == nullptr)
    {
        throw platformError("Failed to create OpenGL context");
    }

    implementation->isGraphicsSurfaceAttached = true;

    if (!SDL_GL_MakeCurrent(implementation->graphicsSurface.platformWindow,
                            implementation->renderingContext))
    {
        shutdown();
        throw platformError("Failed to make OpenGL context current");
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

    beginFrame();

    logger.info("OpenGL 4.6 core Rendering Backend has been initialized.");
}

void OpenGLRenderingBackend::beginFrame()
{
    if (!implementation->isGraphicsSurfaceAttached)
    {
        throw std::runtime_error("Renderer must have an attached Graphics Surface before drawing.");
    }

    if (!SDL_GL_MakeCurrent(implementation->graphicsSurface.platformWindow,
                            implementation->renderingContext))
    {
        throw platformError("Failed to make OpenGL context current");
    }

    implementation->graphicsSurface.graphicsSurfaceSize =
        queryGraphicsSurfaceSize(implementation->graphicsSurface.platformWindow);

    glViewport(0, 0,
               static_cast<GLsizei>(implementation->graphicsSurface.graphicsSurfaceSize.width),
               static_cast<GLsizei>(implementation->graphicsSurface.graphicsSurfaceSize.height));
}

void OpenGLRenderingBackend::clear(LinearColor const &color)
{
    if (!implementation->isGraphicsSurfaceAttached)
    {
        throw std::runtime_error("Renderer must have an attached Graphics Surface before drawing.");
    }

    glClearColor(color.red, color.green, color.blue, color.alpha);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLRenderingBackend::present()
{
    if (!implementation->isGraphicsSurfaceAttached)
    {
        throw std::runtime_error("Renderer must have an attached Graphics Surface before drawing.");
    }

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
    implementation->isGraphicsSurfaceAttached = false;
}

} // namespace Engine::Rendering::Internal
