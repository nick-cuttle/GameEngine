/**
 * @file  Test_InputSystem.cpp
 * @brief Tests frame-based raw input state tracking.
 */

#include <Engine/Input/InputSystem.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("InputSystem", "[unit][input]")
{
    Engine::Input::System input;

    SECTION("tracks key press hold and release across frames")
    {
        // A normal key lifecycle should expose a one-frame pressed flag, preserve held state across
        // beginFrame, then expose a one-frame released flag when the key is released.
        input.beginFrame();
        input.submit(Engine::Input::KeyPressed{.key = Engine::Input::KeyCode::Space});

        REQUIRE(input.isKeyDown(Engine::Input::KeyCode::Space));
        REQUIRE(input.wasKeyPressed(Engine::Input::KeyCode::Space));
        REQUIRE_FALSE(input.wasKeyReleased(Engine::Input::KeyCode::Space));

        input.beginFrame();

        REQUIRE(input.isKeyDown(Engine::Input::KeyCode::Space)); // key still down
        REQUIRE_FALSE(input.wasKeyPressed(Engine::Input::KeyCode::Space));
        REQUIRE_FALSE(input.wasKeyReleased(Engine::Input::KeyCode::Space));

        input.submit(Engine::Input::KeyReleased{.key = Engine::Input::KeyCode::Space});

        REQUIRE_FALSE(input.isKeyDown(Engine::Input::KeyCode::Space));
        REQUIRE_FALSE(input.wasKeyPressed(Engine::Input::KeyCode::Space));
        REQUIRE(input.wasKeyReleased(Engine::Input::KeyCode::Space));
    }

    SECTION("does not mark repeated key presses as new presses while held")
    {
        // Held keys can receive additional press events from the platform. Those should keep the
        // key down without reporting a fresh press in the new frame.
        input.beginFrame();
        input.submit(Engine::Input::KeyPressed{.key = Engine::Input::KeyCode::W});
        input.beginFrame();
        input.submit(Engine::Input::KeyPressed{.key = Engine::Input::KeyCode::W, .repeat = true});

        REQUIRE(input.isKeyDown(Engine::Input::KeyCode::W));
        REQUIRE_FALSE(input.wasKeyPressed(Engine::Input::KeyCode::W));
    }

    SECTION("does not mark explicit key repeats as new presses")
    {
        // SDL-style explicit repeat events should update held state without triggering
        // pressed-this-frame, even if no earlier key-down event was observed.
        input.beginFrame();
        input.submit(Engine::Input::KeyPressed{.key = Engine::Input::KeyCode::D, .repeat = true});

        REQUIRE(input.isKeyDown(Engine::Input::KeyCode::D));
        REQUIRE_FALSE(input.wasKeyPressed(Engine::Input::KeyCode::D));
        REQUIRE_FALSE(input.wasKeyReleased(Engine::Input::KeyCode::D));
    }

    SECTION("ignores invalid key events")
    {
        // Public enum values can be default/sentinel values, and external translators can still
        // produce out-of-range casts. The system must reject all of them before indexing state.
        input.beginFrame();
        input.submit(Engine::Input::KeyPressed{.key = Engine::Input::KeyCode::Unknown});
        input.submit(Engine::Input::KeyReleased{.key = Engine::Input::KeyCode::Unknown});
        input.submit(Engine::Input::KeyPressed{.key = Engine::Input::KeyCode::Count});
        input.submit(Engine::Input::KeyReleased{.key = Engine::Input::KeyCode::Count});
        input.submit(Engine::Input::KeyPressed{.key = static_cast<Engine::Input::KeyCode>(999)});
        input.submit(Engine::Input::KeyReleased{.key = static_cast<Engine::Input::KeyCode>(999)});

        REQUIRE_FALSE(input.isKeyDown(Engine::Input::KeyCode::Unknown));
        REQUIRE_FALSE(input.wasKeyPressed(Engine::Input::KeyCode::Unknown));
        REQUIRE_FALSE(input.wasKeyReleased(Engine::Input::KeyCode::Unknown));
        REQUIRE_FALSE(input.isKeyDown(Engine::Input::KeyCode::Count));
        REQUIRE_FALSE(input.wasKeyPressed(Engine::Input::KeyCode::Count));
        REQUIRE_FALSE(input.wasKeyReleased(Engine::Input::KeyCode::Count));
        REQUIRE_FALSE(input.isKeyDown(static_cast<Engine::Input::KeyCode>(999)));
        REQUIRE_FALSE(input.wasKeyPressed(static_cast<Engine::Input::KeyCode>(999)));
        REQUIRE_FALSE(input.wasKeyReleased(static_cast<Engine::Input::KeyCode>(999)));
    }

    SECTION("does not mark key releases when the key was not down")
    {
        // A release without prior held state should be harmless and should not create a synthetic
        // released-this-frame transition.
        input.beginFrame();
        input.submit(Engine::Input::KeyReleased{.key = Engine::Input::KeyCode::A});

        REQUIRE_FALSE(input.isKeyDown(Engine::Input::KeyCode::A));
        REQUIRE_FALSE(input.wasKeyPressed(Engine::Input::KeyCode::A));
        REQUIRE_FALSE(input.wasKeyReleased(Engine::Input::KeyCode::A));
    }

    SECTION("same-frame key release clears pressed state")
    {
        // If a key is pressed and released in the same frame, the final state is up and only the
        // release transition remains observable.
        input.beginFrame();
        input.submit(Engine::Input::KeyPressed{.key = Engine::Input::KeyCode::S});
        input.submit(Engine::Input::KeyReleased{.key = Engine::Input::KeyCode::S});

        REQUIRE_FALSE(input.isKeyDown(Engine::Input::KeyCode::S));
        REQUIRE_FALSE(input.wasKeyPressed(Engine::Input::KeyCode::S));
        REQUIRE(input.wasKeyReleased(Engine::Input::KeyCode::S));
    }

    SECTION("same-frame key press after release clears released state")
    {
        // If a held key is released and pressed again in the same frame, the final state is down
        // and only the fresh press transition remains observable.
        input.beginFrame();
        input.submit(Engine::Input::KeyPressed{.key = Engine::Input::KeyCode::S});
        input.beginFrame();
        input.submit(Engine::Input::KeyReleased{.key = Engine::Input::KeyCode::S});
        input.submit(Engine::Input::KeyPressed{.key = Engine::Input::KeyCode::S});

        REQUIRE(input.isKeyDown(Engine::Input::KeyCode::S));
        REQUIRE(input.wasKeyPressed(Engine::Input::KeyCode::S));
        REQUIRE_FALSE(input.wasKeyReleased(Engine::Input::KeyCode::S));
    }

    SECTION("tracks mouse button press hold and release across frames")
    {
        // Mouse button state mirrors key state: pressed is transient, held state persists, and
        // released is transient after a valid release.
        input.beginFrame();
        input.submit(Engine::Input::MouseButtonPressed{.button = Engine::Input::MouseButton::Left});

        REQUIRE(input.isMouseButtonDown(Engine::Input::MouseButton::Left));
        REQUIRE(input.wasMouseButtonPressed(Engine::Input::MouseButton::Left));
        REQUIRE_FALSE(input.wasMouseButtonReleased(Engine::Input::MouseButton::Left));

        input.beginFrame();

        REQUIRE(input.isMouseButtonDown(Engine::Input::MouseButton::Left));
        REQUIRE_FALSE(input.wasMouseButtonPressed(Engine::Input::MouseButton::Left));
        REQUIRE_FALSE(input.wasMouseButtonReleased(Engine::Input::MouseButton::Left));

        input.submit(
            Engine::Input::MouseButtonReleased{.button = Engine::Input::MouseButton::Left});

        REQUIRE_FALSE(input.isMouseButtonDown(Engine::Input::MouseButton::Left));
        REQUIRE_FALSE(input.wasMouseButtonPressed(Engine::Input::MouseButton::Left));
        REQUIRE(input.wasMouseButtonReleased(Engine::Input::MouseButton::Left));
    }

    SECTION("does not mark repeated mouse button presses as new presses while held")
    {
        // Repeated button-down events while already held should not create another
        // pressed-this-frame transition.
        input.beginFrame();
        input.submit(
            Engine::Input::MouseButtonPressed{.button = Engine::Input::MouseButton::Right});
        input.beginFrame();
        input.submit(
            Engine::Input::MouseButtonPressed{.button = Engine::Input::MouseButton::Right});

        REQUIRE(input.isMouseButtonDown(Engine::Input::MouseButton::Right));
        REQUIRE_FALSE(input.wasMouseButtonPressed(Engine::Input::MouseButton::Right));
    }

    SECTION("ignores invalid mouse button events")
    {
        // Default mouse button events use Unknown, and external translators can still provide
        // sentinel or out-of-range values. None of those should affect real button state.
        input.beginFrame();
        input.submit(Engine::Input::MouseButtonPressed{});
        input.submit(Engine::Input::MouseButtonReleased{});
        input.submit(
            Engine::Input::MouseButtonPressed{.button = Engine::Input::MouseButton::Count});
        input.submit(
            Engine::Input::MouseButtonReleased{.button = Engine::Input::MouseButton::Count});
        input.submit(Engine::Input::MouseButtonPressed{
            .button = static_cast<Engine::Input::MouseButton>(999)});
        input.submit(Engine::Input::MouseButtonReleased{
            .button = static_cast<Engine::Input::MouseButton>(999)});

        REQUIRE_FALSE(input.isMouseButtonDown(Engine::Input::MouseButton::Unknown));
        REQUIRE_FALSE(input.wasMouseButtonPressed(Engine::Input::MouseButton::Unknown));
        REQUIRE_FALSE(input.wasMouseButtonReleased(Engine::Input::MouseButton::Unknown));
        REQUIRE_FALSE(input.isMouseButtonDown(Engine::Input::MouseButton::Count));
        REQUIRE_FALSE(input.wasMouseButtonPressed(Engine::Input::MouseButton::Count));
        REQUIRE_FALSE(input.wasMouseButtonReleased(Engine::Input::MouseButton::Count));
        REQUIRE_FALSE(input.isMouseButtonDown(static_cast<Engine::Input::MouseButton>(999)));
        REQUIRE_FALSE(input.wasMouseButtonPressed(static_cast<Engine::Input::MouseButton>(999)));
        REQUIRE_FALSE(input.wasMouseButtonReleased(static_cast<Engine::Input::MouseButton>(999)));
        REQUIRE_FALSE(input.isMouseButtonDown(Engine::Input::MouseButton::Left));
    }

    SECTION("does not mark mouse button releases when the button was not down")
    {
        // A release without prior held state should be ignored for mouse buttons just like keys.
        input.beginFrame();
        input.submit(
            Engine::Input::MouseButtonReleased{.button = Engine::Input::MouseButton::Middle});

        REQUIRE_FALSE(input.isMouseButtonDown(Engine::Input::MouseButton::Middle));
        REQUIRE_FALSE(input.wasMouseButtonPressed(Engine::Input::MouseButton::Middle));
        REQUIRE_FALSE(input.wasMouseButtonReleased(Engine::Input::MouseButton::Middle));
    }

    SECTION("same-frame mouse button release clears pressed state")
    {
        // Pressing and releasing a button in one frame leaves the button up and reports only the
        // release transition.
        input.beginFrame();
        input.submit(Engine::Input::MouseButtonPressed{.button = Engine::Input::MouseButton::X1});
        input.submit(Engine::Input::MouseButtonReleased{.button = Engine::Input::MouseButton::X1});

        REQUIRE_FALSE(input.isMouseButtonDown(Engine::Input::MouseButton::X1));
        REQUIRE_FALSE(input.wasMouseButtonPressed(Engine::Input::MouseButton::X1));
        REQUIRE(input.wasMouseButtonReleased(Engine::Input::MouseButton::X1));
    }

    SECTION("same-frame mouse button press after release clears released state")
    {
        // Releasing and pressing a held button in one frame leaves the button down and reports only
        // the press transition.
        input.beginFrame();
        input.submit(Engine::Input::MouseButtonPressed{.button = Engine::Input::MouseButton::X2});
        input.beginFrame();
        input.submit(Engine::Input::MouseButtonReleased{.button = Engine::Input::MouseButton::X2});
        input.submit(Engine::Input::MouseButtonPressed{.button = Engine::Input::MouseButton::X2});

        REQUIRE(input.isMouseButtonDown(Engine::Input::MouseButton::X2));
        REQUIRE(input.wasMouseButtonPressed(Engine::Input::MouseButton::X2));
        REQUIRE_FALSE(input.wasMouseButtonReleased(Engine::Input::MouseButton::X2));
    }

    SECTION("tracks mouse position and accumulates mouse movement per frame")
    {
        // Mouse movement keeps the latest absolute position while accumulating all relative deltas
        // submitted during the frame.
        input.beginFrame();
        input.submit(Engine::Input::MouseMoved{
            .position = {.x = 10.0F, .y = 20.0F},
            .delta = {.x = 3.0F, .y = 4.0F},
        });
        input.submit(Engine::Input::MouseMoved{
            .position = {.x = 15.0F, .y = 18.0F},
            .delta = {.x = 5.0F, .y = -2.0F},
        });

        REQUIRE(input.mousePosition().x == 15.0F);
        REQUIRE(input.mousePosition().y == 18.0F);
        REQUIRE(input.mouseDelta().x == 8.0F);
        REQUIRE(input.mouseDelta().y == 2.0F);

        input.beginFrame();

        REQUIRE(input.mousePosition().x == 15.0F);
        REQUIRE(input.mousePosition().y == 18.0F);
        REQUIRE(input.mouseDelta().x == 0.0F);
        REQUIRE(input.mouseDelta().y == 0.0F);
    }

    SECTION("accumulates scroll per frame")
    {
        // Scroll input is frame-transient and accumulates all wheel movement until beginFrame.
        input.beginFrame();
        input.submit(Engine::Input::MouseWheelScrolled{.delta = {.x = 1.0F, .y = 2.0F}});
        input.submit(Engine::Input::MouseWheelScrolled{.delta = {.x = -0.5F, .y = 3.0F}});

        REQUIRE(input.scrollDelta().x == 0.5F);
        REQUIRE(input.scrollDelta().y == 5.0F);

        input.beginFrame();

        REQUIRE(input.scrollDelta().x == 0.0F);
        REQUIRE(input.scrollDelta().y == 0.0F);
    }
}
