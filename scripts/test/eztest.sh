#!/usr/bin/env sh

set -e

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
if [ -f "$SCRIPT_DIR/../printHelper.sh" ]; then
    . "$SCRIPT_DIR/../printHelper.sh"
else
    . "$SCRIPT_DIR/printHelper.sh"
fi

BUILD_DIR=""
TEST_PATTERN=""
LABEL=""
LIST_TESTS=0

export CTEST_COLOR_DIAGNOSTICS=1

usage() {
    echo "Usage: eztest.sh [build-dir] [options]"
    echo "Example: eztest.sh build/Debug"
    echo "With no build directory, runs build/Debug first, then build/Release."
    echo ""
    echo "Options:"
    echo "  --test <regex>    Run tests matching a CTest regex"
    echo "  --label <regex>   Run tests with matching CTest labels"
    echo "  --list            List discovered tests"
    echo "  --help            Show this help"
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --test)
            if [ -z "$2" ]; then
                echo "Error: --test requires a regex"
                exit 1
            fi
            TEST_PATTERN="$2"
            shift 2
            ;;
        --label)
            if [ -z "$2" ]; then
                echo "Error: --label requires a regex"
                exit 1
            fi
            LABEL="$2"
            shift 2
            ;;
        --list)
            LIST_TESTS=1
            shift
            ;;
        --help)
            usage
            exit 0
            ;;
        -*)
            echo "Unsupported option: $1"
            usage
            exit 1
            ;;
        *)
            if [ -n "$BUILD_DIR" ]; then
                echo "Unsupported argument: $1"
                usage
                exit 1
            fi
            BUILD_DIR="$1"
            shift
            ;;
    esac
done

run_tests() {
    BUILD_DIR="$1"

    if [ ! -d "$BUILD_DIR" ]; then
        echo "Error: build directory does not exist: $BUILD_DIR"
        echo "Run ezbuild.sh $BUILD_DIR first"
        exit 1
    fi

    print_status "$COLOR_BLUE" "Building $BUILD_DIR unit tests"
    cmake --build "$BUILD_DIR" --target EngineUnitTests -- -j$(nproc || echo 8)

    if [ "$LIST_TESTS" -eq 1 ]; then
        print_status "$COLOR_BLUE" "Listing $BUILD_DIR tests"
        ctest --test-dir "$BUILD_DIR" -N
        return
    fi

    print_status "$COLOR_BLUE" "Running $BUILD_DIR tests"

    if [ -n "$TEST_PATTERN" ] && [ -n "$LABEL" ]; then
        ctest --test-dir "$BUILD_DIR" --output-on-failure -R "$TEST_PATTERN" -L "$LABEL"
    elif [ -n "$TEST_PATTERN" ]; then
        ctest --test-dir "$BUILD_DIR" --output-on-failure -R "$TEST_PATTERN"
    elif [ -n "$LABEL" ]; then
        ctest --test-dir "$BUILD_DIR" --output-on-failure -L "$LABEL"
    else
        ctest --test-dir "$BUILD_DIR" --output-on-failure
    fi

    print_status "$COLOR_GREEN" "$BUILD_DIR tests passed"
}

if [ -n "$BUILD_DIR" ]; then
    run_tests "$BUILD_DIR"
else
    run_tests build/Debug
    run_tests build/Release
fi
