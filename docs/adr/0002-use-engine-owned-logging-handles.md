# Use Engine-Owned Logging Handles

As the engine adds subsystems, logging will become a ubiquitous dependency. We will keep concrete logging backend types such as spdlog out of public subsystem interfaces by introducing a core Logging System that owns backend setup and creates lightweight Logger handles for subsystems. This keeps logging policy, backend lifetime, and subsystem naming local while giving every subsystem one common convention for writing logs.

## Considered Options

- Use spdlog directly in subsystem interfaces.
- Keep a thin wrapper that returns shared spdlog logger pointers.
- Use an engine-owned Logging System and Logger handle.

## Consequences

- The Logging System owns sink setup, log file layout, level policy, flush policy, backend lifetime, and duplicate subsystem name handling.
- Subsystems receive Logger handles during composition instead of creating or looking up concrete backend loggers.
- Subsystem Logger names use the subsystem's domain concept name in PascalCase without spaces, such as `WindowSystem` or `InputSystem`.
- Logger should stay concrete, lightweight, and cheap to copy; pluggable logging adapters should wait until there is more than one real logging backend.
