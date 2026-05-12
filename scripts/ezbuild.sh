#!/usr/bin/env sh

set -e

build_dir_arg=$1
COLOR_RESET="$(printf '\033[0m')"
COLOR_BLUE="$(printf '\033[1;34m')"
COLOR_GREEN="$(printf '\033[1;32m')"
COLOR_YELLOW="$(printf '\033[1;33m')"
COLOR_RED="$(printf '\033[1;31m')"

export CLICOLOR_FORCE=1

usage() {
    echo "Usage: ./scripts/ezbuild.sh [build-dir]"
    echo "Example: ./scripts/ezbuild.sh build/Debug"
    echo "With no build directory, builds build/Debug and build/Release."
    echo "The CMake build type is inferred from Debug or Release in the build directory name."
    echo ""
    echo "Environment:"
    echo "  VCPKG_ROOT                 Enables vcpkg manifest mode when set."
    echo "  VCPKG_TARGET_TRIPLET       Defaults to x64-mingw-dynamic on MSYS/MinGW."
    echo "  VCPKG_HOST_TRIPLET         Defaults to VCPKG_TARGET_TRIPLET on MSYS/MinGW."
    echo "  VCPKG_DEFAULT_BINARY_CACHE Defaults to .vcpkg-cache when VCPKG_ROOT is set."
    echo "  ENGINE_CMAKE_GENERATOR     Overrides the selected CMake generator."
}

case "$build_dir_arg" in
    --help|-h)
        usage
        exit 0
        ;;
esac

system_name="$(uname -s)"

# Prefer Ninja when available because it gives faster incremental builds on both Linux and Windows.
if [ -n "$ENGINE_CMAKE_GENERATOR" ]; then
    generator="$ENGINE_CMAKE_GENERATOR"
elif command -v ninja >/dev/null 2>&1; then
    generator="Ninja"
elif echo "$system_name" | grep Linux >/dev/null; then
    generator="Unix Makefiles"
elif echo "$system_name" | grep -E 'MSYS|MINGW' >/dev/null; then
    generator="MinGW Makefiles"
else
    echo "Unsupported operating system: $system_name"
    exit 1
fi

cmake_args="-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
using_vcpkg=0

print_status() {
    color=$1
    message=$2
    printf "%s==> %s%s\n" "$color" "$message" "$COLOR_RESET"
}

if [ -n "$VCPKG_ROOT" ]; then
    using_vcpkg=1
    # vcpkg must be provided as a CMake toolchain during configure, before project() runs.
    vcpkg_toolchain="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

    if [ ! -f "$vcpkg_toolchain" ]; then
        print_status "$COLOR_RED" "VCPKG_ROOT is set, but the vcpkg CMake toolchain file was not found."
        echo "VCPKG_ROOT value: $VCPKG_ROOT"
        echo "Expected file: $vcpkg_toolchain"
        echo "Check that VCPKG_ROOT points to the vcpkg checkout, not its parent directory."
        echo "Example MSYS value: export VCPKG_ROOT=/c/Users/angry/Dev/vcpkg"
        exit 1
    fi

    cmake_args="$cmake_args -DCMAKE_TOOLCHAIN_FILE=$vcpkg_toolchain"

    if echo "$system_name" | grep -E 'MSYS|MINGW' >/dev/null; then
        # MinGW needs explicit vcpkg triplets; otherwise vcpkg tends to choose Visual Studio ones.
        : "${VCPKG_TARGET_TRIPLET:=x64-mingw-dynamic}"
        : "${VCPKG_HOST_TRIPLET:=$VCPKG_TARGET_TRIPLET}"
        export VCPKG_TARGET_TRIPLET
        export VCPKG_HOST_TRIPLET
        cmake_args="$cmake_args -DVCPKG_TARGET_TRIPLET=$VCPKG_TARGET_TRIPLET"
        cmake_args="$cmake_args -DVCPKG_HOST_TRIPLET=$VCPKG_HOST_TRIPLET"
    fi

    if [ -z "$VCPKG_DEFAULT_BINARY_CACHE" ]; then
        # Keep the default cache visible and local to the repo; users can override with a shared path.
        export VCPKG_DEFAULT_BINARY_CACHE="$(pwd)/.vcpkg-cache"
    fi
    mkdir -p "$VCPKG_DEFAULT_BINARY_CACHE"

    echo "Using VCPKG_ROOT: $VCPKG_ROOT"
    echo "Using VCPKG_DEFAULT_BINARY_CACHE: $VCPKG_DEFAULT_BINARY_CACHE"
    if [ -n "$VCPKG_TARGET_TRIPLET" ]; then
        echo "Using VCPKG_TARGET_TRIPLET: $VCPKG_TARGET_TRIPLET"
    fi
    if [ -n "$VCPKG_HOST_TRIPLET" ]; then
        echo "Using VCPKG_HOST_TRIPLET: $VCPKG_HOST_TRIPLET"
    fi
fi

summarize_vcpkg_dependencies() {
    configure_log=$1

    [ "$using_vcpkg" -eq 1 ] || return 0
    [ -f "$configure_log" ] || return 0

    for dependency in catch2 fmt sdl3 spdlog; do
        if grep -E "Restored .* package\(s\)" "$configure_log" >/dev/null &&
            grep -E "(Installing|Removing) [0-9]+/[0-9]+ ${dependency}(:|\\[)" "$configure_log" >/dev/null; then
            print_status "$COLOR_GREEN" "vcpkg restored $dependency from binary cache"
        elif grep -E "Building ${dependency}(:|\\[)" "$configure_log" >/dev/null; then
            print_status "$COLOR_YELLOW" "vcpkg built $dependency from source"
        elif grep -E "^    ${dependency}(:|\\[)" "$configure_log" >/dev/null &&
            grep -F "The following packages are already installed:" "$configure_log" >/dev/null; then
            print_status "$COLOR_GREEN" "vcpkg already had $dependency installed"
        else
            print_status "$COLOR_YELLOW" "vcpkg did not report cache status for $dependency"
        fi
    done
}

is_mingw_vcpkg_build() {
    configure_log=$1

    {
        echo "$VCPKG_TARGET_TRIPLET $VCPKG_HOST_TRIPLET"
        [ -f "$configure_log" ] && grep -i mingw "$configure_log"
    } | grep -i mingw >/dev/null
}

print_configure_log() {
    configure_log=$1

    if is_mingw_vcpkg_build "$configure_log"; then
        awk -v blue="$COLOR_BLUE" -v reset="$COLOR_RESET" '
            /^-- Running vcpkg install$/ {
                print blue "-- Running vcpkg install (details in configure log)" reset
                in_vcpkg = 1
                next
            }
            /^-- Running vcpkg install - done$/ {
                print blue $0 reset
                in_vcpkg = 0
                next
            }
            !in_vcpkg {
                print blue $0 reset
            }
        ' "$configure_log"
    else
        awk -v blue="$COLOR_BLUE" -v reset="$COLOR_RESET" '{ print blue $0 reset }' "$configure_log"
    fi
}

build() {
    build_dir=$1
    build_name=${build_dir%/}
    build_name=${build_name##*/}

    if [ -z "$build_name" ]; then
        echo "Invalid build directory: $build_dir"
        exit 1
    fi

    case "$build_name" in
        *Debug*)
            type=Debug
            ;;
        *Release*)
            type=Release
            ;;
        *)
            echo "Could not infer CMake build type from build directory: $build_dir"
            echo "Include Debug or Release in the build directory name."
            exit 1
            ;;
    esac

    print_status "$COLOR_BLUE" "Configuring $build_dir ($type)"
    print_status "$COLOR_BLUE" "Using CMake generator: $generator"

    configure_log="$build_dir/ezbuild-configure.log"
    mkdir -p "$build_dir"

    # cmake_args intentionally remains word-split so each -D option is passed separately.
    # shellcheck disable=SC2086
    if ! cmake -G "$generator" -S . -B "$build_dir" -DCMAKE_BUILD_TYPE="$type" $cmake_args >"$configure_log" 2>&1; then
        cat "$configure_log"
        print_status "$COLOR_RED" "CMake configuration failed for $build_dir. Check the log for details: $configure_log"
        exit 1
    fi
    print_configure_log "$configure_log"

    summarize_vcpkg_dependencies "$configure_log"
    print_status "$COLOR_BLUE" "Building $build_dir"

    if [ "$generator" = "Ninja" ]; then
        cmake --build "$build_dir"
    else
        cmake --build "$build_dir" -- -j"$(nproc || echo 8)"
    fi

    print_status "$COLOR_GREEN" "$build_dir build passed"
}

# Run builds
case "$build_dir_arg" in
    "")
        build build/Debug
        build build/Release
        ;;
    *)
        build "$build_dir_arg"
        ;;
esac
