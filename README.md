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
- `clang-format` for `ezformat.sh`
- `gcovr` for `ezcoverage.sh`

The engine code targets Linux and Windows. The helper scripts prefer Ninja when it is available,
then fall back to Unix Makefiles on Linux and MinGW Makefiles on MSYS or MinGW shells. Set
`ENGINE_CMAKE_GENERATOR` to override the generated build system.

## Helper Scripts

Prepare shell commands for the moved script layout:

```bash
source scripts/core/ezprepare.sh
```

This publishes the nested `ez*.sh` helpers into the ignored `scripts/scripts` command directory and
prepends that directory to `PATH`. Linux shells receive symbolic links; MSYS, MinGW, and Cygwin
shells receive small executable shims so Windows symlink privileges are not required.

## Building

The helper script configures and builds into the provided build directory:

```bash
ezbuild.sh build/Debug
```

The CMake build type is inferred from `Debug` or `Release` in the final directory name, so paths
like `build/Debug_myBuild` and `build/NickRelease` work. With no build directory, the script builds
both `build/Debug` and `build/Release`.

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

The `ezrun.sh` helper defaults to `build/Release` and accepts a build directory argument.
It checks for both `GameEngine` and `GameEngine.exe`.

```bash
ezrun.sh build/Debug
```

## Testing

Build first, then run unit tests with:

```bash
eztest.sh build/Debug
```

With no build directory, `eztest.sh` runs `build/Debug` and then `build/Release`. Useful filters:

```bash
eztest.sh build/Debug --list
eztest.sh build/Debug --label unit
eztest.sh build/Debug --test WindowSystem
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
ezformat.sh
```

Generate coverage reports with GCC coverage instrumentation:

```bash
ezcoverage.sh build/Coverage/Debug
```

Coverage output is written to `coverage/index.html` and `coverage/coverage.xml`.

With no build directory, `ezcoverage.sh` runs `build/Coverage/Debug` and then
`build/Coverage/Release` before combining the report. Pass `--test <regex>`, `--label <regex>`, or
`--open` for focused coverage runs and automatic report opening.

Run the build matrix across the main CMake option combinations:

```bash
ezgaunlet.sh
```

This builds Debug and Release variants while toggling `BUILD_TESTS` and
`ENGINE_WARNINGS_AS_ERRORS`. Use `ezgaunlet.sh --build-root <dir>` to write the matrix builds
outside the default `build/Gaunlet` directory.

## Project Layout

- `src/Engine/Core`: engine paths and engine-owned logging handles.
- `src/Engine/Runtime`: context object that composes engine-wide services.
- `src/Engine/Windowing`: SDL-backed window lifecycle and window event translation.
- `src/Engine/Input`: engine-owned input codes, events, and per-frame input state.
- `src/main.cpp`: executable startup and primary-window runtime loop.
- `tests/unit`: Catch2 unit tests registered with CTest.
- `tests/support`: shared test harness helpers.
- `scripts/build`: build configuration helpers, build matrix checks, and commit helper.
- `scripts/core`: shared script output helpers and `ezprepare.sh` command publishing.
- `scripts/coverage`: coverage build, test, and report generation.
- `scripts/format`: source formatting helper.
- `scripts/run`: executable launcher.
- `scripts/test`: CTest wrapper and shared test-script helper.
- `docs/dependencies.md`: platform dependency setup and vcpkg notes.
- `docs/adr`: durable architectural decisions.

## Notes

This project is in its early stages. Renderer implementation is not present yet; renderer-related
language in `CONTEXT.md` and ADRs describes the intended subsystem boundary.
