/**
 * @file  GraphicsSurfaceFactory.hpp
 * @brief Declares the internal bridge from engine windows to renderer graphics surfaces.
 */

#pragma once

#include <Engine/Windowing/WindowSystem.hpp>

struct SDL_Window;

namespace Engine::Rendering::Internal
{

/// @brief Internal platform graphics surface data consumed by concrete Rendering Backends.
/// @details This type is intentionally kept outside public Renderer APIs. It may carry platform
///          details because only engine implementation files include it.
struct PlatformGraphicsSurface
{
    /// @brief Platform window that owns the backend graphics surface.
    SDL_Window *platformWindow = nullptr;
    /// @brief Current drawable pixel size for the graphics surface.
    GraphicsSurfaceSize graphicsSurfaceSize{};
};

/// @brief Internal bridge that creates backend-specific graphics surface attachments.
/// @details The factory keeps SDL window lookup inside engine implementation code and prevents
///          public Renderer APIs from exposing SDL windows or native handles.
class GraphicsSurfaceFactory
{
public:
    /// @brief Creates surface data for the OpenGL Rendering Backend.
    /// @param windowSystem Window System that owns the target window.
    /// @param windowIdentifier Engine-owned target window identifier.
    /// @return Platform surface data required by the OpenGL Rendering Backend.
    /// @throws std::runtime_error when the Window System is not initialized or the platform cannot
    ///         report the drawable graphics surface size.
    /// @throws std::invalid_argument when the window identifier is not managed by the Window
    ///         System.
    static PlatformGraphicsSurface createOpenGLGraphicsSurface(WindowSystem &windowSystem,
                                                               WindowIdentifier windowIdentifier);
};

} // namespace Engine::Rendering::Internal
