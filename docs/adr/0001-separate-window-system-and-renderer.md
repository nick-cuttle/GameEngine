# Separate Window System And Renderer

We will keep the SDL-backed Window System separate from the Renderer, keep SDL types out of public engine headers, and connect rendering backends to windows through an internal Graphics Surface Factory. This avoids spreading direct OpenGL calls through runtime/window code and makes Vulkan a new Rendering Backend with its own surface attachment path instead of a redesign of the windowing architecture.

## Considered Options

- Expose SDL handles directly to renderer/application code for simplicity.
- Let the Window System own OpenGL context creation, Vulkan surface creation, and presentation.
- Use a single graphics module that combines window ownership and rendering backend commands.

## Consequences

- Window creation must declare a Graphics Surface Capability before a renderer attaches to it.
- Renderer backends own presentation and graphics surface lifetime.
- The Runtime Loop must release renderer surfaces before destroying their windows.
