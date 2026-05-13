#!/usr/bin/env sh

BUILD_HELPER_DIR="$(CDPATH= cd -- "$(dirname -- "${BUILD_HELPER_PATH:-$0}")" && pwd)"

# load printHelpers.sh
if [ -f "$BUILD_HELPER_DIR/../core/printHelper.sh" ]; then
    . "$BUILD_HELPER_DIR/../core/printHelper.sh"
else
    . "$BUILD_HELPER_DIR/printHelper.sh"
fi

# Global variables for build scripts
BUILD_HELPER_SYSTEM_NAME=""
BUILD_HELPER_GENERATOR=""
BUILD_HELPER_CMAKE_ARGS=""
BUILD_HELPER_USING_VCPKG=0
BUILD_HELPER_JOBS=""

enable_default_vcpkg_binary_cache() {
    if [ -z "$VCPKG_DEFAULT_BINARY_CACHE" ]; then
        export VCPKG_DEFAULT_BINARY_CACHE="$(pwd)/.vcpkg-cache"
    fi
    mkdir -p "$VCPKG_DEFAULT_BINARY_CACHE"
}

# Initializes common variables for build-family scripts. This should be called at the start of each script
init_build_helper() {
    BUILD_HELPER_SYSTEM_NAME="$(uname -s)"
    BUILD_HELPER_CMAKE_ARGS="-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    BUILD_HELPER_USING_VCPKG=0
    BUILD_HELPER_JOBS="$(nproc 2>/dev/null || echo 8)"

    # Prefer Ninja for fast incremental builds, but keep Makefile fallbacks for plain Linux/MinGW
    # environments where Ninja is not installed.
    if [ -n "$ENGINE_CMAKE_GENERATOR" ]; then
        BUILD_HELPER_GENERATOR="$ENGINE_CMAKE_GENERATOR"
    elif command -v ninja >/dev/null 2>&1; then
        BUILD_HELPER_GENERATOR="Ninja"
    elif echo "$BUILD_HELPER_SYSTEM_NAME" | grep Linux >/dev/null; then
        BUILD_HELPER_GENERATOR="Unix Makefiles"
    elif echo "$BUILD_HELPER_SYSTEM_NAME" | grep -E 'MSYS|MINGW' >/dev/null; then
        BUILD_HELPER_GENERATOR="MinGW Makefiles"
    else
        echo "Unsupported operating system: $BUILD_HELPER_SYSTEM_NAME"
        return 1
    fi

    # When vcpkg is present, wire it in before CMake's project() runs. MinGW needs explicit triplets
    # because vcpkg otherwise tends to select Visual Studio defaults on Windows.
    if [ -n "$VCPKG_ROOT" ]; then
        BUILD_HELPER_USING_VCPKG=1
        vcpkg_toolchain="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

        if [ ! -f "$vcpkg_toolchain" ]; then
            print_status "$COLOR_RED" "VCPKG_ROOT is set, but the vcpkg CMake toolchain file was not found."
            echo "VCPKG_ROOT value: $VCPKG_ROOT"
            echo "Expected file: $vcpkg_toolchain"
            return 1
        fi

        BUILD_HELPER_CMAKE_ARGS="$BUILD_HELPER_CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=$vcpkg_toolchain"

        # Special handling for MinGW/vcpkg builds to set triplets and binary cache directory
        if echo "$BUILD_HELPER_SYSTEM_NAME" | grep -E 'MSYS|MINGW' >/dev/null; then
            : "${VCPKG_TARGET_TRIPLET:=x64-mingw-dynamic}"
            : "${VCPKG_HOST_TRIPLET:=$VCPKG_TARGET_TRIPLET}"
            export VCPKG_TARGET_TRIPLET
            export VCPKG_HOST_TRIPLET
            BUILD_HELPER_CMAKE_ARGS="$BUILD_HELPER_CMAKE_ARGS -DVCPKG_TARGET_TRIPLET=$VCPKG_TARGET_TRIPLET"
            BUILD_HELPER_CMAKE_ARGS="$BUILD_HELPER_CMAKE_ARGS -DVCPKG_HOST_TRIPLET=$VCPKG_HOST_TRIPLET"
        fi

        # Default cache location to current directory if not environment variable.
        enable_default_vcpkg_binary_cache
    fi
}

append_cmake_definition() {
    name=$1
    value=$2
    current=$3

    printf "%s -D%s=%s" "$current" "$name" "$value"
}

# Determines whether a build directory is Debug or Release based on its name.
infer_build_type_from_dir() {
    build_dir=$1
    build_name=${build_dir%/}
    build_name=${build_name##*/}

   # Contains Debug, debug build, Release, release build
    case "$build_name" in
        *Debug*) printf "Debug" ;;
        *Release*) printf "Release" ;;
        *)
            echo "Could not infer CMake build type from build directory: $build_dir" >&2
            echo "Include Debug or Release in the build directory name." >&2
            return 1
            ;;
    esac
}

# check if a mingw build has vcpkg dependencies
is_mingw_vcpkg_build() {
    configure_log=$1

    {
        echo "$VCPKG_TARGET_TRIPLET $VCPKG_HOST_TRIPLET"
        [ -f "$configure_log" ] && grep -i mingw "$configure_log"
    } | grep -i mingw >/dev/null
}

print_configure_log() {
    configure_log=$1

    # Reduce configure outout from vcpkg
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
            !in_vcpkg { print blue $0 reset }
        ' "$configure_log"
    else
        awk -v blue="$COLOR_BLUE" -v reset="$COLOR_RESET" '{ print blue $0 reset }' "$configure_log"
    fi
}

# Shorten vcpkg dependency status messages
summarize_vcpkg_dependencies() {
    configure_log=$1

    [ -f "$configure_log" ] || return 0
    if [ "$BUILD_HELPER_USING_VCPKG" -ne 1 ] && ! grep -F -- "-- Running vcpkg install" "$configure_log" >/dev/null; then
        return 0
    fi

    # Common dependencies to check for
    for dependency in catch2 fmt glad sdl3 spdlog; do
        if grep -E "Building ${dependency}(:|\\[)" "$configure_log" >/dev/null; then
            print_status "$COLOR_YELLOW" "vcpkg built $dependency from source"
        elif grep -E "Restored .* package\(s\)" "$configure_log" >/dev/null &&
            grep -E "(Installing|Removing) [0-9]+/[0-9]+ ${dependency}(:|\\[)" "$configure_log" >/dev/null; then
            print_status "$COLOR_GREEN" "vcpkg restored $dependency from binary cache"
        elif grep -E "^    ${dependency}(:|\\[)" "$configure_log" >/dev/null &&
            grep -F "The following packages are already installed:" "$configure_log" >/dev/null; then
            print_status "$COLOR_GREEN" "vcpkg already had $dependency installed"
        else
            print_status "$COLOR_YELLOW" "vcpkg did not report cache status for $dependency"
        fi
    done
}

configure_build() {
    build_dir=$1
    build_type=$2
    configure_log=$3
    extra_cmake_args=$4

    mkdir -p "$build_dir"
    cmake_cache="$build_dir/CMakeCache.txt"
    if [ "$BUILD_HELPER_USING_VCPKG" -ne 1 ] && [ -f "$cmake_cache" ] &&
        grep -i "vcpkg.*buildsystems.*vcpkg.cmake" "$cmake_cache" >/dev/null; then
        BUILD_HELPER_USING_VCPKG=1
        enable_default_vcpkg_binary_cache
    fi

    print_status "$COLOR_BLUE" "Using CMake generator: $BUILD_HELPER_GENERATOR"

    # BUILD_HELPER_CMAKE_ARGS and extra_cmake_args intentionally remain word-split so each -D flag
    # is passed as its own CMake argument. Do not quote these variables as a single string.
    # shellcheck disable=SC2086
    if ! cmake -G "$BUILD_HELPER_GENERATOR" -S . -B "$build_dir" -DCMAKE_BUILD_TYPE="$build_type" $BUILD_HELPER_CMAKE_ARGS $extra_cmake_args >"$configure_log" 2>&1; then
        cat "$configure_log"
        return 1
    fi

    print_configure_log "$configure_log"
    summarize_vcpkg_dependencies "$configure_log"
}

build_configured_dir() {
    build_dir=$1
    target=$2

    # Ninja manages parallelism itself; Makefile generators need an explicit job count.
    if [ -n "$target" ]; then
        cmake --build "$build_dir" --target "$target" -- -j"$BUILD_HELPER_JOBS"
    elif [ "$BUILD_HELPER_GENERATOR" = "Ninja" ]; then
        cmake --build "$build_dir"
    else
        cmake --build "$build_dir" -- -j"$BUILD_HELPER_JOBS"
    fi
}
