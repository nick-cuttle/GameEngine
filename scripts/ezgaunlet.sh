#!/usr/bin/env sh

set -e

COLOR_RESET="$(printf '\033[0m')"
COLOR_BLUE="$(printf '\033[1;34m')"
COLOR_GREEN="$(printf '\033[1;32m')"
COLOR_YELLOW="$(printf '\033[1;33m')"
COLOR_RED="$(printf '\033[1;31m')"

export CLICOLOR_FORCE=1
export CTEST_COLOR_DIAGNOSTICS=1

BUILD_ROOT="build/Gaunlet"
INCLUDE_SDL_FETCH_VARIANTS=0
RUN_TESTS=0

usage() {
    echo "Usage: ./scripts/ezgaunlet.sh [options]"
    echo ""
    echo "Builds the engine across a CMake option matrix:"
    echo "  CMAKE_BUILD_TYPE: Debug, Release"
    echo "  BUILD_TESTS: ON, OFF"
    echo "  ENGINE_WARNINGS_AS_ERRORS: ON, OFF"
    echo "  ENGINE_FETCH_DEPENDENCIES: ON, OFF"
    echo ""
    echo "Options:"
    echo "  --build-root <dir>             Defaults to build/Gaunlet"
    echo "  --include-sdl-fetch-variants   Also toggles SDL_FORCE_FETCH, ENGINE_ENABLE_SDL_GPU,"
    echo "                                 and ENGINE_ENABLE_SDL_VULKAN"
    echo "  --test                         Run CTest for variants built with BUILD_TESTS=ON"
    echo "  --help                         Show this help"
    echo ""
    echo "Environment:"
    echo "  VCPKG_ROOT                 Enables vcpkg manifest mode when set."
    echo "  VCPKG_TARGET_TRIPLET       Defaults to x64-mingw-dynamic on MSYS/MinGW."
    echo "  VCPKG_HOST_TRIPLET         Defaults to VCPKG_TARGET_TRIPLET on MSYS/MinGW."
    echo "  VCPKG_DEFAULT_BINARY_CACHE Defaults to .vcpkg-cache when VCPKG_ROOT is set."
    echo "  ENGINE_CMAKE_GENERATOR     Overrides the selected CMake generator."
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --build-root)
            if [ -z "$2" ]; then
                echo "Error: --build-root requires a directory"
                exit 1
            fi
            BUILD_ROOT="$2"
            shift 2
            ;;
        --include-sdl-fetch-variants)
            INCLUDE_SDL_FETCH_VARIANTS=1
            shift
            ;;
        --test)
            RUN_TESTS=1
            shift
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            echo "Unsupported option: $1"
            usage
            exit 1
            ;;
    esac
done

system_name="$(uname -s)"

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

JOBS="$(nproc 2>/dev/null || echo 8)"
cmake_base_args="-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
using_vcpkg=0

print_status() {
    color=$1
    message=$2
    printf "%s==> %s%s\n" "$color" "$message" "$COLOR_RESET"
}

append_on_off() {
    name=$1
    value=$2
    current=$3

    printf "%s -D%s=%s" "$current" "$name" "$value"
}

slug_on_off() {
    name=$1
    value=$2

    if [ "$value" = "ON" ]; then
        printf "%sOn" "$name"
    else
        printf "%sOff" "$name"
    fi
}

if [ -n "$VCPKG_ROOT" ]; then
    using_vcpkg=1
    vcpkg_toolchain="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

    if [ ! -f "$vcpkg_toolchain" ]; then
        print_status "$COLOR_RED" "VCPKG_ROOT is set, but the vcpkg CMake toolchain file was not found."
        echo "VCPKG_ROOT value: $VCPKG_ROOT"
        echo "Expected file: $vcpkg_toolchain"
        exit 1
    fi

    cmake_base_args="$cmake_base_args -DCMAKE_TOOLCHAIN_FILE=$vcpkg_toolchain"

    if echo "$system_name" | grep -E 'MSYS|MINGW' >/dev/null; then
        : "${VCPKG_TARGET_TRIPLET:=x64-mingw-dynamic}"
        : "${VCPKG_HOST_TRIPLET:=$VCPKG_TARGET_TRIPLET}"
        export VCPKG_TARGET_TRIPLET
        export VCPKG_HOST_TRIPLET
        cmake_base_args="$cmake_base_args -DVCPKG_TARGET_TRIPLET=$VCPKG_TARGET_TRIPLET"
        cmake_base_args="$cmake_base_args -DVCPKG_HOST_TRIPLET=$VCPKG_HOST_TRIPLET"
    fi

    if [ -z "$VCPKG_DEFAULT_BINARY_CACHE" ]; then
        export VCPKG_DEFAULT_BINARY_CACHE="$(pwd)/.vcpkg-cache"
    fi
    mkdir -p "$VCPKG_DEFAULT_BINARY_CACHE"
fi

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

run_variant() {
    build_type=$1
    build_tests=$2
    warnings_as_errors=$3
    fetch_dependencies=$4
    sdl_force_fetch=$5
    sdl_gpu=$6
    sdl_vulkan=$7

    variant_name="${build_type}_$(slug_on_off Tests "$build_tests")_$(slug_on_off Werror "$warnings_as_errors")_$(slug_on_off FetchDeps "$fetch_dependencies")"
    cmake_args="$cmake_base_args"
    cmake_args="$(append_on_off BUILD_TESTS "$build_tests" "$cmake_args")"
    cmake_args="$(append_on_off ENGINE_WARNINGS_AS_ERRORS "$warnings_as_errors" "$cmake_args")"
    cmake_args="$(append_on_off ENGINE_FETCH_DEPENDENCIES "$fetch_dependencies" "$cmake_args")"

    if [ "$INCLUDE_SDL_FETCH_VARIANTS" -eq 1 ]; then
        variant_name="${variant_name}_$(slug_on_off SdlFetch "$sdl_force_fetch")_$(slug_on_off SdlGpu "$sdl_gpu")_$(slug_on_off SdlVulkan "$sdl_vulkan")"
        cmake_args="$(append_on_off SDL_FORCE_FETCH "$sdl_force_fetch" "$cmake_args")"
        cmake_args="$(append_on_off ENGINE_ENABLE_SDL_GPU "$sdl_gpu" "$cmake_args")"
        cmake_args="$(append_on_off ENGINE_ENABLE_SDL_VULKAN "$sdl_vulkan" "$cmake_args")"
    fi

    build_dir="$BUILD_ROOT/$variant_name"
    configure_log="$build_dir/ezgaunlet-configure.log"
    mkdir -p "$build_dir"

    print_status "$COLOR_BLUE" "Configuring $variant_name"
    print_status "$COLOR_BLUE" "Build directory: $build_dir"

    # cmake_args intentionally remains word-split so each -D option is passed separately.
    # shellcheck disable=SC2086
    if ! cmake -G "$generator" -S . -B "$build_dir" -DCMAKE_BUILD_TYPE="$build_type" $cmake_args >"$configure_log" 2>&1; then
        cat "$configure_log"
        print_status "$COLOR_RED" "$variant_name configure failed. Check $configure_log"
        return 1
    fi

    print_configure_log "$configure_log"
    summarize_vcpkg_dependencies "$configure_log"

    print_status "$COLOR_BLUE" "Building $variant_name"
    if [ "$generator" = "Ninja" ]; then
        if ! cmake --build "$build_dir"; then
            print_status "$COLOR_RED" "$variant_name build failed"
            return 1
        fi
    else
        if ! cmake --build "$build_dir" -- -j"$JOBS"; then
            print_status "$COLOR_RED" "$variant_name build failed"
            return 1
        fi
    fi

    if [ "$RUN_TESTS" -eq 1 ] && [ "$build_tests" = "ON" ]; then
        print_status "$COLOR_BLUE" "Running tests for $variant_name"
        if ! ctest --test-dir "$build_dir" --output-on-failure; then
            print_status "$COLOR_RED" "$variant_name tests failed"
            return 1
        fi
    fi

    print_status "$COLOR_GREEN" "$variant_name passed"
}

failures=""
total=0
passed=0

print_status "$COLOR_BLUE" "Starting build gaunlet in $BUILD_ROOT"
print_status "$COLOR_BLUE" "Using CMake generator: $generator"

for build_type in Debug Release; do
    for build_tests in ON OFF; do
        for warnings_as_errors in ON OFF; do
            for fetch_dependencies in ON OFF; do
                if [ "$INCLUDE_SDL_FETCH_VARIANTS" -eq 1 ]; then
                    for sdl_force_fetch in ON OFF; do
                        for sdl_gpu in ON OFF; do
                            for sdl_vulkan in ON OFF; do
                                total=$((total + 1))
                                if run_variant "$build_type" "$build_tests" "$warnings_as_errors" "$fetch_dependencies" "$sdl_force_fetch" "$sdl_gpu" "$sdl_vulkan"; then
                                    passed=$((passed + 1))
                                else
                                    failures="$failures
$build_type BUILD_TESTS=$build_tests ENGINE_WARNINGS_AS_ERRORS=$warnings_as_errors ENGINE_FETCH_DEPENDENCIES=$fetch_dependencies SDL_FORCE_FETCH=$sdl_force_fetch ENGINE_ENABLE_SDL_GPU=$sdl_gpu ENGINE_ENABLE_SDL_VULKAN=$sdl_vulkan"
                                fi
                            done
                        done
                    done
                else
                    total=$((total + 1))
                    if run_variant "$build_type" "$build_tests" "$warnings_as_errors" "$fetch_dependencies" OFF OFF OFF; then
                        passed=$((passed + 1))
                    else
                        failures="$failures
$build_type BUILD_TESTS=$build_tests ENGINE_WARNINGS_AS_ERRORS=$warnings_as_errors ENGINE_FETCH_DEPENDENCIES=$fetch_dependencies"
                    fi
                fi
            done
        done
    done
done

if [ "$passed" -eq "$total" ]; then
    print_status "$COLOR_GREEN" "Build gaunlet passed: $passed/$total variants"
else
    print_status "$COLOR_RED" "Build gaunlet failed: $passed/$total variants passed"
    echo "Failed variants:$failures"
    exit 1
fi
