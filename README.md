# GameEngine

A C++23 custom game engine project. The current codebase provides an engine static library, a
small executable that opens the primary window, and focused unit tests for core runtime systems.

For project vocabulary, subsystem boundaries, and architectural decisions, start with
[`CONTEXT.md`](CONTEXT.md) and [`docs/adr/`](docs/adr/).

## Requirements

- CMake 3.16 or newer
- A C++23 compiler
- Git, because dependencies are fetched with CMake `FetchContent`
- A POSIX-compatible shell for the helper scripts
- `clang-format` for `scripts/ezformat.sh`
- `gcovr` for `scripts/ezcoverage.sh`

The engine code targets Linux and Windows. The helper scripts select Unix Makefiles on Linux and
MinGW Makefiles on MSYS or MinGW shells. For other Windows generators, use the equivalent CMake
commands directly.

## Building

The helper script configures and builds under `build/<build_type>`:

```bash
./scripts/ezbuild.sh <build_type>
```

`build_type` can be `Debug`, `Release`, or omitted to build both configurations.

Equivalent direct CMake commands:

```bash
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build/Debug
```

Common CMake options:

- `ENGINE_WARNINGS_AS_ERRORS`: treats compiler warnings as errors. Defaults to `ON`.
- `BUILD_TESTS`: builds the Catch2 unit test target when CTest is enabled. Defaults to `ON`.
- `ENGINE_FETCH_DEPENDENCIES`: fetches missing dependencies from source after `find_package`
  lookup fails. Defaults to `ON`.
- `SDL_FORCE_FETCH`: always fetches SDL3 from source instead of using an installed SDL3 package.
  Defaults to `OFF`.
- `ENGINE_ENABLE_SDL_GPU`: enables SDL GPU support when fetching SDL. Defaults to `OFF`.
- `ENGINE_ENABLE_SDL_VULKAN`: enables SDL Vulkan support when fetching SDL. Defaults to `OFF`.

## Dependencies

The project first looks for installed packages and then falls back to CMake `FetchContent` when
`ENGINE_FETCH_DEPENDENCIES` is enabled. This keeps clone-and-build working while letting developer
machines use faster package-manager installs.

On Windows with MSYS/MinGW, use vcpkg manifest mode by setting `VCPKG_ROOT`. On Linux, prefer
distro packages when they are available. See [`docs/dependencies.md`](docs/dependencies.md) for the
full setup guide.

## Running

After building, run the executable from the chosen configuration:

```bash
./build/Debug/bin/GameEngine
```

On MinGW-style Windows builds, the executable name is usually `GameEngine.exe`. The
`scripts/ezrun.sh` helper currently expects that `.exe` name.

## Testing

Build first, then run unit tests with:

```bash
./scripts/eztest.sh Debug
```

With no build type, `eztest.sh` runs `Debug` and then `Release`. Useful filters:

```bash
./scripts/eztest.sh Debug --list
./scripts/eztest.sh Debug --label unit
./scripts/eztest.sh Debug --test WindowSystem
```

CTest discovers the Catch2 test cases from `EngineUnitTests`. Current unit tests cover:

- core paths and logging
- runtime context initialization
- SDL-backed window lifecycle and event translation
- frame-based input state
- shared test support helpers

## Formatting And Coverage

Format C++ source and headers:

```bash
./scripts/ezformat.sh
```

Generate coverage reports with GCC coverage instrumentation:

```bash
./scripts/ezcoverage.sh Debug
```

Coverage output is written to `coverage/index.html` and `coverage/coverage.xml`.

## Project Layout

- `src/Engine/Core`: engine paths and engine-owned logging handles.
- `src/Engine/Runtime`: context object that composes engine-wide services.
- `src/Engine/Windowing`: SDL-backed window lifecycle and window event translation.
- `src/Engine/Input`: engine-owned input codes, events, and per-frame input state.
- `src/main.cpp`: executable startup and primary-window runtime loop.
- `tests/unit`: Catch2 unit tests registered with CTest.
- `tests/support`: shared test harness helpers.
- `scripts`: local build, test, format, coverage, run, and path helpers.
- `docs/dependencies.md`: platform dependency setup and vcpkg notes.
- `docs/adr`: durable architectural decisions.

## Notes

This project is in its early stages. Renderer implementation is not present yet; renderer-related
language in `CONTEXT.md` and ADRs describes the intended subsystem boundary.
