/**
 * @file  Renderer.hpp
 * @brief Declares the backend-neutral renderer facade and frame presentation types.
 */

#pragma once

#include <Engine/Core/Logger.hpp>
#include <Engine/Windowing/WindowSystem.hpp>

#include <memory>

namespace Engine
{

/// @brief Four-channel color value expressed in linear color space.
/// @details Values are passed directly to the active Rendering Backend for clear operations. The
///          Renderer does not clamp values so higher-level color policy can remain explicit.
struct LinearColor
{
    /// @brief Linear red channel value.
    float red = 0.0F;
    /// @brief Linear green channel value.
    float green = 0.0F;
    /// @brief Linear blue channel value.
    float blue = 0.0F;
    /// @brief Linear alpha channel value.
    float alpha = 1.0F;
};

/// @brief Concrete Rendering Backend requested by Renderer initialization.
enum class RenderingBackendSelection
{
    /// @brief Use the OpenGL Rendering Backend.
    OpenGL,
    /// @brief Request Vulkan. This is reserved and currently reports a clear not-implemented error.
    Vulkan
};

/// @brief Presentation timing policy used when the Rendering Backend swaps a frame.
enum class PresentationMode
{
    /// @brief Present immediately without waiting for display synchronization.
    Immediate,
    /// @brief Present in sync with the display when the platform supports it.
    VerticalSynchronization
};

/// @brief Engine-owned Renderer setup values.
/// @details The configuration is explicit about backend selection and presentation timing so the
///          application does not depend on a hard-coded rendering backend.
struct RendererConfiguration
{
    /// @brief Rendering Backend used to create graphics contexts and present frames.
    RenderingBackendSelection renderingBackendSelection = RenderingBackendSelection::OpenGL;
    /// @brief Presentation timing requested from the active Rendering Backend.
    PresentationMode presentationMode = PresentationMode::VerticalSynchronization;
};

/// @brief Public facade used by application code to render frames without owning backend commands.
/// @details Renderer hides concrete Rendering Backend implementations, Graphics Surface attachment
///          details, GLAD loading, and OpenGL commands behind engine-owned types.
class Renderer
{
public:
    /// @brief Constructs an empty Renderer facade.
    Renderer();

    /// @brief Releases the active Rendering Backend if one is initialized.
    ~Renderer();

    /// @brief Renderers uniquely own their active Rendering Backend.
    Renderer(Renderer const &) = delete;
    /// @brief Renderers cannot be copy-assigned because they own backend state.
    Renderer &operator=(Renderer const &) = delete;

    /// @brief Initializes the Renderer with the selected concrete backend.
    /// @param configuration Backend and presentation policy requested by the application.
    /// @param logger Logger used for Renderer and backend lifecycle messages.
    /// @throws std::runtime_error when the selected backend is unavailable or the Renderer is
    ///         already initialized.
    void initialize(RendererConfiguration const &configuration, Logger logger);

    /// @brief Attaches the Renderer to a Window System managed graphics surface.
    /// @param windowSystem Window System that owns the target window.
    /// @param windowIdentifier Engine-owned identifier of the target window.
    /// @throws std::runtime_error when the Renderer is not initialized or the backend cannot create
    ///         a graphics context for the window.
    /// @throws std::invalid_argument when the window identifier is not managed by the Window
    ///         System.
    void attachGraphicsSurface(WindowSystem &windowSystem, WindowIdentifier windowIdentifier);

    /// @brief Detaches the Renderer from a Window System managed graphics surface.
    /// @param windowIdentifier Engine-owned identifier of the target window.
    /// @details Detach before destroying the Window System window that owns the Graphics Surface.
    void detachGraphicsSurface(WindowIdentifier windowIdentifier) noexcept;

    /// @brief Records a Window Event that may affect the attached graphics surface.
    /// @param windowEvent Engine-owned Window Event observed by the Window System.
    void handleWindowEvent(WindowEvent const &windowEvent);

    /// @brief Starts rendering work for a frame on the attached graphics surface.
    /// @return True when rendering and presentation should continue for this frame.
    /// @throws std::runtime_error when the Renderer is not initialized, no graphics surface is
    ///         attached, or the backend cannot make its context current.
    bool beginFrame();

    /// @brief Clears the attached graphics surface to the specified linear color.
    /// @param color Linear color used for the clear operation.
    /// @throws std::runtime_error when the Renderer is not initialized or no graphics surface is
    ///         attached.
    void clear(LinearColor const &color);

    /// @brief Presents the current frame to the attached graphics surface.
    /// @throws std::runtime_error when the Renderer is not initialized, no graphics surface is
    ///         attached, or the backend cannot present the frame.
    void present();

    /// @brief Releases active Rendering Backend resources.
    /// @details This operation is idempotent. It must run before the Window System destroys the
    ///          window that owns the attached graphics surface.
    void shutdown() noexcept;

private:
    /// @brief Concrete implementation hidden from consumers of this public header.
    struct Implementation;
    /// @brief Owned implementation state for the Renderer facade.
    std::unique_ptr<Implementation> implementation;
};

} // namespace Engine
