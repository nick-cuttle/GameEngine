/**
 * @file  RenderingBackend.hpp
 * @brief Declares the internal interface implemented by concrete Rendering Backends.
 */

#pragma once

#include <Engine/Rendering/Renderer.hpp>

namespace Engine::Rendering::Internal
{

/// @brief Internal interface for concrete Rendering Backend implementations.
/// @details Public application code calls Renderer. Concrete backends own graphics context setup,
///          frame lifecycle commands, clear commands, and presentation details behind this
///          implementation-only interface.
class RenderingBackend
{
public:
    /// @brief Destroys the backend implementation.
    virtual ~RenderingBackend() = default;

    /// @brief Attaches backend state to a Window System managed graphics surface.
    /// @param windowSystem Window System that owns the target window.
    /// @param windowIdentifier Engine-owned target window identifier.
    virtual void attachGraphicsSurface(WindowSystem &windowSystem,
                                       WindowIdentifier windowIdentifier) = 0;

    /// @brief Detaches backend state from a Window System managed graphics surface.
    /// @param windowIdentifier Engine-owned target window identifier.
    virtual void detachGraphicsSurface(WindowIdentifier windowIdentifier) noexcept = 0;

    /// @brief Records a Window Event that may affect backend-owned surface state.
    /// @param windowEvent Engine-owned Window Event observed by the Window System.
    virtual void handleWindowEvent(WindowEvent const &windowEvent) = 0;

    /// @brief Starts rendering work for the current frame.
    /// @return True when the attached graphics surface can be rendered this frame.
    virtual bool beginFrame() = 0;

    /// @brief Clears the attached graphics surface.
    /// @param color Linear color used for the clear operation.
    virtual void clear(LinearColor const &color) = 0;

    /// @brief Presents the current frame.
    virtual void present() = 0;

    /// @brief Releases backend resources without throwing.
    virtual void shutdown() noexcept = 0;
};

} // namespace Engine::Rendering::Internal
