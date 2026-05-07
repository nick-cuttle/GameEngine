# Game Engine

This context describes the core runtime concepts for a custom game engine.

## Language

**Window System**:
A subsystem that owns application windows, display-related events, and the creation hooks needed for rendering backends to attach to a window.
_Avoid_: Graphics module, renderer

**Renderer**:
A subsystem that owns graphics backend commands, frame submission, and presentation coordination.
_Avoid_: Window system

**Rendering Backend**:
A concrete implementation of rendering behavior for a specific graphics application programming interface.
_Avoid_: Renderer, graphics module

**Graphics Surface**:
A backend attachment point that allows a **Renderer** to present into a window without making the **Window System** own rendering commands.
_Avoid_: Window, render context

**Graphics Surface Factory**:
A renderer-facing bridge that creates backend attachment points for windows without exposing platform-specific windowing details.
_Avoid_: SDL accessor, native handle provider

**Graphics Surface Capability**:
A declaration that a window is intended to support a specific kind of **Graphics Surface**.
_Avoid_: Renderer type, backend setting

**Window Size**:
The logical dimensions of a window as understood by the application and desktop environment.
_Avoid_: Framebuffer size, swapchain extent

**Graphics Surface Size**:
The pixel dimensions of the **Graphics Surface** that a **Renderer** presents into.
_Avoid_: Window size, framebuffer size, swapchain extent

**Primary Window**:
The default window used by an application when it has only one main presentation target.
_Avoid_: Main window, single window

**Window Event**:
An engine-owned description of a window or display state change observed by the **Window System**.
_Avoid_: Platform event, callback

**Window Identifier**:
A stable engine-owned reference to a window managed by the **Window System**.
_Avoid_: Window pointer, backend handle

**Close Request**:
A **Window Event** indicating that a window has been asked to close but has not necessarily been destroyed.
_Avoid_: Quit, destroy

**Input System**:
A subsystem that owns keyboard, mouse, controller, and other player input state and events.
_Avoid_: Window system input

**Runtime Loop**:
The per-frame coordinator that polls systems, advances frame work, and applies shutdown policy.
_Avoid_: Window loop, renderer loop

**Unit Test**:
A focused test that verifies one engine type or function with controlled inputs and no required engine runtime.
_Avoid_: Small test, low-level test

**Integration Test**:
A test that verifies multiple engine systems working together through public engine APIs.
_Avoid_: System test, runtime test

**Test Harness**:
Shared test support that creates isolated resources, configures systems, or runs bounded engine scenarios.
_Avoid_: Fixture, test utility

**Smoke Test**:
A coarse test that verifies a built executable or high-level scenario starts and exits successfully.
_Avoid_: Sanity test, launch test

## Relationships

- A **Window System** creates and manages one or more application windows.
- A **Window System** may designate one **Primary Window** while still supporting additional windows.
- A **Window Identifier** refers to exactly one window managed by the **Window System** while that window exists.
- A window has a **Graphics Surface Capability** when it is expected to be used for rendering.
- A **Graphics Surface Size** may differ from the **Window Size** on high-density displays.
- A **Window System** produces **Window Events** as a per-frame batch during event polling.
- **Window Events** initially cover close requests, movement, **Window Size** changes, **Graphics Surface Size** changes, focus changes, minimized or restored state, and display scale changes.
- A **Close Request** does not destroy a window by itself.
- A **Close Request** for the **Primary Window** requests application shutdown by default.
- A **Close Request** for a non-primary window closes only that window by default.
- The **Input System** owns input concerns even when platform input events arrive through the same polling source as **Window Events**.
- The **Runtime Loop** coordinates the **Window System** and **Renderer** each frame.
- A **Renderer** presents frames through a **Graphics Surface** associated with a window.
- A **Renderer** may present through more than one **Graphics Surface**.
- A **Renderer** obtains **Graphics Surfaces** through a **Graphics Surface Factory**.
- A **Renderer** uses a **Rendering Backend** such as OpenGL today or Vulkan later.
- A **Renderer** owns presentation operations for its **Graphics Surface**.
- A **Window System** must not own backend rendering commands.
- A **Renderer** must release a **Graphics Surface** before the **Window System** destroys the associated window.
- A **Unit Test** should not require the **Runtime Loop**.
- An **Integration Test** may use real filesystem resources only through a **Test Harness** that creates a unique isolated temporary directory for each test.
- A **Smoke Test** verifies startup behavior without replacing focused **Unit Tests** or **Integration Tests**.
- Each test file should expose one overall Catch2 test case named after the file, such as `PathsTests`, with detailed scenarios expressed as subcases.
- Each overall Catch2 test case is registered as an individual CTest test.
- Test targets are controlled by the CMake `BUILD_TESTS` option.

## Example Dialogue

> **Dev:** "Should the window code call OpenGL directly?"
> **Domain expert:** "No. The **Window System** exposes the attachment point; the **Renderer** owns OpenGL or Vulkan work."

> **Dev:** "Can an integration test write logs to disk?"
> **Domain expert:** "Yes, but only through a **Test Harness** that owns a unique isolated temporary directory for that test."

## Flagged Ambiguities

- "windowing system" was used near rendering concerns; resolved: **Window System** and **Renderer** are separate subsystems.
- The **Window System** may prepare a window for a **Graphics Surface Capability**, but backend attachment work belongs behind the renderer-facing bridge.
- "one window" was discussed as an initial runtime need; resolved: the model supports multiple windows with a **Primary Window** convenience path.
- "testing infrastructure" was broad; resolved: start with **Unit Tests** for the Engine library and extend through **Integration Tests** using a **Test Harness**.
- CTest should discover overall Catch2 test cases by test-file identity instead of exposing each scenario as a separate top-level test.
