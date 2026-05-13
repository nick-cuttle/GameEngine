/**
 * @file Test_Renderer.cpp
 * @brief Tests public Renderer configuration and facade lifecycle behavior.
 */

#include <Engine/Rendering/Renderer.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

TEST_CASE("Renderer", "[unit][rendering][renderer]")
{
    SECTION("renderer configuration selects OpenGL and vertical synchronization by default")
    {
        Engine::RendererConfiguration configuration;

        REQUIRE(configuration.renderingBackendSelection ==
                Engine::RenderingBackendSelection::OpenGL);
        REQUIRE(configuration.presentationMode ==
                Engine::PresentationMode::VerticalSynchronization);
    }

    SECTION("linear color defaults to opaque black")
    {
        Engine::LinearColor color;

        REQUIRE(color.red == 0.0F);
        REQUIRE(color.green == 0.0F);
        REQUIRE(color.blue == 0.0F);
        REQUIRE(color.alpha == 1.0F);
    }

    SECTION("vulkan selection fails without a Vulkan dependency")
    {
        Engine::Renderer renderer;
        Engine::RendererConfiguration configuration;
        configuration.renderingBackendSelection = Engine::RenderingBackendSelection::Vulkan;

        REQUIRE_THROWS_WITH(
            renderer.initialize(configuration, Engine::Logger{}),
            Catch::Matchers::Equals("Vulkan Rendering Backend is not implemented."));
    }

    SECTION("drawing requires an initialized renderer")
    {
        Engine::Renderer renderer;

        REQUIRE_THROWS_WITH(
            renderer.beginFrame(),
            Catch::Matchers::Equals("Renderer must be initialized before drawing."));
        REQUIRE_THROWS_WITH(
            renderer.clear(Engine::LinearColor{}),
            Catch::Matchers::Equals("Renderer must be initialized before drawing."));
        REQUIRE_THROWS_WITH(
            renderer.present(),
            Catch::Matchers::Equals("Renderer must be initialized before drawing."));
    }

    SECTION("drawing requires an attached graphics surface")
    {
        Engine::Renderer renderer;
        Engine::RendererConfiguration configuration;

        renderer.initialize(configuration, Engine::Logger{});

        REQUIRE_THROWS_WITH(renderer.beginFrame(),
                            Catch::Matchers::Equals(
                                "Renderer must have an attached Graphics Surface before drawing."));
        REQUIRE_THROWS_WITH(renderer.clear(Engine::LinearColor{}),
                            Catch::Matchers::Equals(
                                "Renderer must have an attached Graphics Surface before drawing."));
        REQUIRE_THROWS_WITH(renderer.present(),
                            Catch::Matchers::Equals(
                                "Renderer must have an attached Graphics Surface before drawing."));

        renderer.shutdown();
    }
}
