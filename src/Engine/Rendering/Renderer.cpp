/**
 * @file  Renderer.cpp
 * @brief Implements the backend-neutral Renderer facade.
 */

#include "Renderer.hpp"

#include <Engine/Rendering/Internal/OpenGLRenderingBackend.hpp>
#include <Engine/Rendering/Internal/RenderingBackend.hpp>

#include <memory>
#include <stdexcept>

namespace
{

/// @brief Creates the concrete Rendering Backend requested by configuration.
/// @param configuration Renderer setup values.
/// @param logger Logger used by the created backend.
/// @return Owned concrete Rendering Backend.
/// @throws std::runtime_error when the selected backend is not implemented.
std::unique_ptr<Engine::Rendering::Internal::RenderingBackend>
createRenderingBackend(Engine::RendererConfiguration const &configuration, Engine::Logger logger)
{
    switch (configuration.renderingBackendSelection)
    {
    case Engine::RenderingBackendSelection::OpenGL:
        return std::make_unique<Engine::Rendering::Internal::OpenGLRenderingBackend>(
            logger, configuration.presentationMode);
    case Engine::RenderingBackendSelection::Vulkan:
        throw std::runtime_error("Vulkan Rendering Backend is not implemented.");
    }

    throw std::runtime_error("Unsupported Rendering Backend selection.");
}

} // namespace

namespace Engine
{

struct Renderer::Implementation
{
    /// @brief Active concrete Rendering Backend.
    std::unique_ptr<Rendering::Internal::RenderingBackend> renderingBackend;
    /// @brief Renderer facade logger.
    Logger logger;
};

Renderer::Renderer() : implementation(std::make_unique<Implementation>())
{
}

Renderer::~Renderer()
{
    shutdown();
}

void Renderer::initialize(RendererConfiguration const &configuration, Logger logger)
{
    if (implementation->renderingBackend != nullptr)
    {
        throw std::runtime_error("Renderer is already initialized.");
    }

    implementation->logger = logger;
    implementation->renderingBackend = createRenderingBackend(configuration, logger);
    implementation->logger.info("Renderer has been initialized.");
}

void Renderer::attachGraphicsSurface(WindowSystem &windowSystem, WindowIdentifier windowIdentifier)
{
    if (implementation->renderingBackend == nullptr)
    {
        throw std::runtime_error(
            "Renderer must be initialized before attaching a Graphics Surface.");
    }

    implementation->renderingBackend->attachGraphicsSurface(windowSystem, windowIdentifier);
}

void Renderer::detachGraphicsSurface(WindowIdentifier windowIdentifier) noexcept
{
    if (implementation->renderingBackend == nullptr)
    {
        return;
    }

    implementation->renderingBackend->detachGraphicsSurface(windowIdentifier);
}

void Renderer::handleWindowEvent(WindowEvent const &windowEvent)
{
    if (implementation->renderingBackend == nullptr)
    {
        return;
    }

    implementation->renderingBackend->handleWindowEvent(windowEvent);
}

bool Renderer::beginFrame()
{
    if (implementation->renderingBackend == nullptr)
    {
        throw std::runtime_error("Renderer must be initialized before drawing.");
    }

    return implementation->renderingBackend->beginFrame();
}

void Renderer::clear(LinearColor const &color)
{
    if (implementation->renderingBackend == nullptr)
    {
        throw std::runtime_error("Renderer must be initialized before drawing.");
    }

    implementation->renderingBackend->clear(color);
}

void Renderer::present()
{
    if (implementation->renderingBackend == nullptr)
    {
        throw std::runtime_error("Renderer must be initialized before drawing.");
    }

    implementation->renderingBackend->present();
}

void Renderer::shutdown() noexcept
{
    if (implementation->renderingBackend == nullptr)
    {
        return;
    }

    implementation->renderingBackend->shutdown();
    implementation->renderingBackend.reset();
}

} // namespace Engine
