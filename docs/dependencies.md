# Dependencies

GameEngine supports three dependency paths:

1. Installed packages found by CMake `find_package`.
2. vcpkg manifest mode when `VCPKG_ROOT` is set.
3. CMake `FetchContent` fallback when packages are missing and `ENGINE_FETCH_DEPENDENCIES=ON`.

The normal recommendation is:

- Windows with MSYS/MinGW: use vcpkg.
- Linux: use distro packages when available.
- CI or strict version testing: use vcpkg or FetchContent for pinned dependency versions.

## Windows vcpkg Setup

Install vcpkg outside the repo. This example uses `C:\Users\angry\Dev\vcpkg`; any path is fine as
long as `VCPKG_ROOT` points to the vcpkg checkout itself.

PowerShell:

```powershell
mkdir C:\Users\angry\Dev
git clone https://github.com/microsoft/vcpkg C:\Users\angry\Dev\vcpkg
C:\Users\angry\Dev\vcpkg\bootstrap-vcpkg.bat
```

MSYS/Git Bash:

```bash
export VCPKG_ROOT=/c/Users/angry/Dev/vcpkg
```

To make that permanent, add it to `~/.bashrc`:

```bash
export VCPKG_ROOT=/c/Users/angry/Dev/vcpkg
```

Reload the shell configuration:

```bash
source ~/.bashrc
```

Verify:

```bash
test -f "$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" && echo "vcpkg ok"
```

## Windows Build

With `VCPKG_ROOT` set, the helper script automatically enables vcpkg manifest mode:

```bash
./scripts/ezbuild.sh Debug
```

On MSYS/MinGW, the script defaults these triplets:

```bash
VCPKG_TARGET_TRIPLET=x64-mingw-dynamic
VCPKG_HOST_TRIPLET=x64-mingw-dynamic
```

This avoids vcpkg selecting the Visual Studio default triplet on a MinGW build.

## Binary Cache

When vcpkg is enabled, `ezbuild.sh` uses `.vcpkg-cache` by default:

```bash
export VCPKG_DEFAULT_BINARY_CACHE="$(pwd)/.vcpkg-cache"
```

The first cache-enabled build may still build dependencies because it has to populate the cache.
Later fresh build directories should restore package archives from that cache instead of rebuilding
dependencies from source.

Use a shared cache if you want multiple repos to reuse the same vcpkg package archives:

```bash
export VCPKG_DEFAULT_BINARY_CACHE=/c/Users/angry/Dev/vcpkg-cache
```

## Linux Setup

On Linux, prefer distro packages when they are available. Example package names vary by distro and
release.

Arch:

```bash
sudo pacman -S cmake gcc ninja sdl3 fmt spdlog catch2
./scripts/ezbuild.sh Debug
```

Ubuntu/Debian, when SDL3 packages are available:

```bash
sudo apt install cmake g++ ninja-build libsdl3-dev libfmt-dev libspdlog-dev catch2
./scripts/ezbuild.sh Debug
```

If a distro package is missing or too old, leave `ENGINE_FETCH_DEPENDENCIES=ON` and CMake will
fetch the pinned dependency from source.

## Direct CMake Configure

The helper script is preferred, but direct CMake works too.

Windows MSYS/MinGW with vcpkg:

```bash
export VCPKG_ROOT=/c/Users/angry/Dev/vcpkg
cmake -G Ninja -S . -B build/Debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic \
  -DVCPKG_HOST_TRIPLET=x64-mingw-dynamic \
  -DBUILD_TESTS=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build/Debug
```

Linux system packages:

```bash
cmake -G Ninja -S . -B build/Debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTS=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build/Debug
```

## Dependency Versions

The pinned fallback and vcpkg manifest versions are:

- SDL3 `3.4.8`
- fmt `12.1.0`
- spdlog `1.17.0`
- Catch2 `3.14.0`

Linux system packages do not need to match these exact versions as long as they provide compatible
CMake targets. If a package cannot be found, the fallback path uses the pinned source version.
