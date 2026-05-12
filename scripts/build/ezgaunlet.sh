#!/usr/bin/env sh

set -e

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"

# Include buildHelper.sh for functions
if [ -f "$SCRIPT_DIR/buildHelper.sh" ]; then
    BUILD_HELPER_PATH="$SCRIPT_DIR/buildHelper.sh"
    . "$SCRIPT_DIR/buildHelper.sh"
else
    BUILD_HELPER_PATH="$SCRIPT_DIR/../build/buildHelper.sh"
    . "$SCRIPT_DIR/../build/buildHelper.sh"
fi

BUILD_ROOT="build/Gaunlet"

usage() {
    echo "Usage: ezgaunlet.sh [--build-root <dir>]"
}

# Parse parameters
while [ "$#" -gt 0 ]; do
    case "$1" in
        --build-root)
            BUILD_ROOT="$2"
            shift 2
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

# init build helper to set common variables and detect vcpkg
init_build_helper

# Build each supported CMake option combination in its own directory. Keeping directories separate
# avoids stale cache values hiding configuration-specific failures.
for build_type in Debug Release; do
    for build_tests in ON OFF; do
        for warnings_as_errors in ON OFF; do

            # make directory name
            build_dir="$BUILD_ROOT/${build_type}_Tests${build_tests}_Werror${warnings_as_errors}"
            configure_log="$build_dir/ezgaunlet-configure.log"

            # append cmake arguments
            cmake_args=""
            cmake_args="$(append_cmake_definition BUILD_TESTS "$build_tests" "$cmake_args")"
            cmake_args="$(append_cmake_definition ENGINE_WARNINGS_AS_ERRORS "$warnings_as_errors" "$cmake_args")"

            # Configure and build
            print_status "$COLOR_BLUE" "Configuring $build_dir"
            configure_build "$build_dir" "$build_type" "$configure_log" "$cmake_args"
            print_status "$COLOR_BLUE" "Building $build_dir"
            build_configured_dir "$build_dir" ""
        done
    done
done

print_status "$COLOR_GREEN" "Build gaunlet passed"
