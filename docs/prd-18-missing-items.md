# PRD #18 Missing Implementation Items

The following PRD #18 items were identified as missing from the current implementation. Manual smoke verification is intentionally excluded because it is tracked separately in issue #25.

1. `WindowConfiguration` is missing borderless fullscreen, decorated, and high-density options from PRD stories 12-13. Current config only has title, size, visible, resizable, and graphics surface capability in `src/Engine/Windowing/WindowSystem.hpp`.

2. `RendererConfiguration` is missing sRGB preference and debug options. Current config only selects backend and presentation mode in `src/Engine/Rendering/Renderer.hpp`.

3. sRGB-capable OpenGL surface/default path is not implemented. No `SDL_GL_FRAMEBUFFER_SRGB_CAPABLE`, `GL_FRAMEBUFFER_SRGB`, or sRGB configuration exists in `src/`.

4. OpenGL debug output for debug builds is not implemented. No debug context flag, debug callback, or `GL_DEBUG_OUTPUT` setup exists.

5. GLAD is still generated during build via CMake, not committed as generated sources. `cmake/Dependencies.cmake` still configures `glad_add_library`, which conflicts with the PRD requirement that normal builds should not depend on GLAD generation.

6. `Context` does not own the Window System service, despite the PRD implementation decision. `Context` owns paths and logging only in `src/Engine/Runtime/Context.hpp`, while `main.cpp` creates `WindowSystem` directly.
