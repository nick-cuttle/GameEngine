#!/usr/bin/env sh

set -e

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
# The script can run either from scripts/build or through a generated scripts/scripts shim.
if [ -f "$SCRIPT_DIR/buildHelper.sh" ]; then
    BUILD_HELPER_PATH="$SCRIPT_DIR/buildHelper.sh"
    . "$SCRIPT_DIR/buildHelper.sh"
else
    BUILD_HELPER_PATH="$SCRIPT_DIR/../build/buildHelper.sh"
    . "$SCRIPT_DIR/../build/buildHelper.sh"
fi

build_dir_arg=$1

usage() {
    echo "Usage: ezbuild.sh [build-dir]"
    echo "Example: ezbuild.sh build/Debug"
    echo "With no build directory, builds build/Debug and build/Release."
    echo "The CMake build type is inferred from Debug or Release in the build directory name."
}

case "$build_dir_arg" in
    --help|-h)
        usage
        exit 0
        ;;
esac

init_build_helper

build() {
    build_dir=$1
    build_type=$(infer_build_type_from_dir "$build_dir") || exit 1
    configure_log="$build_dir/ezbuild-configure.log"

    print_status "$COLOR_BLUE" "Configuring $build_dir ($build_type)"
    if ! configure_build "$build_dir" "$build_type" "$configure_log" ""; then
        print_status "$COLOR_RED" "CMake configuration failed for $build_dir. Check the log for details: $configure_log"
        exit 1
    fi

    print_status "$COLOR_BLUE" "Building $build_dir"
    build_configured_dir "$build_dir" ""
    print_status "$COLOR_GREEN" "$build_dir build passed"
}

case "$build_dir_arg" in
    "")
        build build/Debug
        build build/Release
        ;;
    *)
        build "$build_dir_arg"
        ;;
esac
