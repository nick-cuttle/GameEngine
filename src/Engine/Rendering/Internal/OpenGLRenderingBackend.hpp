/**
 * @file  OpenGLRenderingBackend.hpp
 * @brief Declares the internal OpenGL Rendering Backend implementation.
 */

#pragma once

#include <Engine/Rendering/Internal/RenderingBackend.hpp>

#include <memory>

namespace Engine::Rendering::Internal
{

/// @brief Rendering Backend that owns OpenGL context setup, GLAD loading, and OpenGL commands.
class OpenGLRenderingBackend final : public RenderingBackend
{
public:
    /// @brief Constructs an OpenGL backend with the requested presentation policy.
    /// @param logger Logger used for backend lifecycle messages.
    /// @param presentationMode Presentation timing requested for frame swaps.
    OpenGLRenderingBackend(Logger logger, PresentationMode presentationMode);

    /// @brief Releases the OpenGL context if it is still attached.
    ~OpenGLRenderingBackend() override;

    /// @brief OpenGL backends uniquely own their graphics context.
    OpenGLRenderingBackend(OpenGLRenderingBackend const &) = delete;
    /// @brief OpenGL backends cannot be copy-assigned because they own context state.
    OpenGLRenderingBackend &operator=(OpenGLRenderingBackend const &) = delete;

    /// @copydoc RenderingBackend::attachGraphicsSurface
    void attachGraphicsSurface(WindowSystem &windowSystem,
                               WindowIdentifier windowIdentifier) override;

    /// @copydoc RenderingBackend::beginFrame
    void beginFrame() override;

    /// @copydoc RenderingBackend::clear
    void clear(LinearColor const &color) override;

    /// @copydoc RenderingBackend::present
    void present() override;

    /// @copydoc RenderingBackend::shutdown
    void shutdown() noexcept override;

private:
    /// @brief SDL and OpenGL context state hidden from this internal header.
    struct Implementation;
    /// @brief Owned backend implementation state.
    std::unique_ptr<Implementation> implementation;
    /// @brief Logger used for backend lifecycle messages.
    Logger logger;
    /// @brief Presentation timing requested for frame swaps.
    PresentationMode presentationMode;
};

} // namespace Engine::Rendering::Internal
