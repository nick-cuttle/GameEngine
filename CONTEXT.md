# Game Engine

This context describes the core runtime concepts for a custom game engine.

## Purpose

GameEngine is a C++23 engine skeleton that currently owns core startup services, an SDL-backed
Window System, platform-neutral input state, and focused unit test infrastructure. The repository
builds a static `Engine` library and a small `GameEngine` executable that composes the current
systems.

## Architecture Map

- `src/Engine/Core` contains cross-cutting services. `Paths` resolves the executable-derived base
  path, logs directory, and assets directory. `LoggingSystem` owns spdlog setup and issues
  engine-owned `Logger` handles.
- `src/Engine/Runtime` contains `Context`, the startup composition point for engine-wide paths and
  logging.
- `src/Engine/Rendering` contains `Renderer`, the backend-neutral rendering facade, plus private
  Rendering Backend implementations and the internal Graphics Surface Factory bridge.
- `src/Engine/Windowing` contains `WindowSystem`, the SDL boundary for video initialization,
  primary-window creation, shutdown, and translation from SDL window events to engine-owned
  `WindowEvent` payloads.
- `src/Engine/Input` contains platform-neutral input codes, input event payloads, and
  `Engine::Input::System`, which tracks held state plus per-frame transitions and deltas.
- `src/main.cpp` initializes `Context`, creates the `WindowSystem` logger, opens the Primary
  Window, then polls window events until the platform requests quit or the Primary Window receives
  a Close Request.
- `tests/unit` contains Catch2 unit tests discovered by CTest. `tests/support` contains reusable
  Test Harness helpers for temporary directories, logging state isolation, log assertions, and
  console capture.
- `docs/adr` records durable architectural decisions. ADR 0001 keeps the Window System separate
  from the Renderer. ADR 0002 keeps concrete logging backend types out of public subsystem
  interfaces.

## Important Flows

**Startup**:
`main()` creates `Engine::Context`, which initializes `Paths` first and then initializes
`LoggingSystem` with the resolved logs directory. `main()` creates subsystem loggers, initializes
the Window System, creates the Primary Window with an OpenGL Graphics Surface Capability,
initializes the Renderer with an explicit Rendering Backend and Presentation Mode, attaches the
Primary Window through the internal Graphics Surface Factory, and enters the Runtime Loop.

**First frame rendering**:
Application code drives rendering through the concrete `Renderer` facade. The OpenGL Rendering
Backend privately creates the graphics context, loads GLAD, clears a `LinearColor`, and presents
the Primary Window. Vulkan can be selected in `RendererConfiguration`, but it fails clearly as not
implemented and does not add a Vulkan build dependency.

**Window event polling**:
`WindowSystem::pollWindowEvents()` drains the SDL event queue. Application quit requests are
reported separately from window-specific events. Only events for managed engine windows are
translated into `WindowEvent` values; unmanaged and unsupported events are ignored.

**Input state updates**:
`Engine::Input::System::beginFrame()` clears transient pressed, released, movement, and scroll
state while preserving held keys, held mouse buttons, and the latest mouse position.
`submit()` accepts only engine-owned input events and rejects sentinel or out-of-range key and
mouse button values before indexing internal state.

**Logging lifecycle**:
`LoggingSystem::initialize()` creates the log directory, installs console and file sinks, registers
the root `Engine` logger, and writes `Engine.log`. Subsystems receive `Logger` handles from
`createSubsystemLogger()`. Issued handles reject writes after `LoggingSystem::shutdown()`.

## Development Workflows

- Build with `./scripts/ezbuild.sh Debug`, `./scripts/ezbuild.sh Release`, or no argument for both.
- Run tests with `./scripts/eztest.sh Debug`; use `--list`, `--label <regex>`, and
  `--test <regex>` for discovery and focused runs.
- Format C++ source and headers with `./scripts/ezformat.sh`.
- Generate coverage with `./scripts/ezcoverage.sh Debug`; this requires `gcovr`.
- Helper scripts support Linux and MSYS or MinGW-style Windows shells. The CMake project itself is
  the cross-platform source of truth.

## Testing Strategy

- Unit tests should target one public engine type or helper at a time and should not require the
  Runtime Loop.
- Window System tests use SDL's dummy video backend when they need SDL initialization without a
  display.
- Tests that need filesystem output should use `TestTempDirectory` so temporary data stays isolated
  under `tests/tmp`.
- Tests that initialize logging should use `SpdlogTestGuard` so spdlog's process-global registry is
  reset before and after the test.
- Log assertion helpers compare stable log message content after removing timestamp prefixes.

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

**Logging System**:
A core subsystem that owns engine logging policy, concrete logging backend setup, and scoped **Logger** creation for engine subsystems.
_Avoid_: spdlog registry, logger factory

**Logger**:
A lightweight engine-owned handle used by subsystems to write log messages without depending on concrete logging backend types.
_Avoid_: spdlog logger, shared logger pointer

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
- The **Logging System** owns logging backend configuration, sink setup, level policy, flush policy, and subsystem logger naming.
- Subsystems receive **Loggers** during composition instead of creating concrete logging backend instances.
- Subsystem **Logger** names should use the subsystem's domain concept name in PascalCase without spaces, such as `WindowSystem` or `InputSystem`.
- Concrete logging backend types should not appear in public subsystem interfaces.
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
