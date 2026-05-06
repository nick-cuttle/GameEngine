#!/usr/bin/env sh

set -e

BUILD_TYPE="Debug"
TEST_PATTERN=""
LABEL=""
LIST_TESTS=0

usage() {
    echo "Usage: ./scripts/eztest.sh [Debug|Release] [options]"
    echo ""
    echo "Options:"
    echo "  --test <regex>    Run tests matching a CTest regex"
    echo "  --label <regex>   Run tests with matching CTest labels"
    echo "  --list            List discovered tests"
    echo "  --help            Show this help"
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        Debug|Release)
            BUILD_TYPE="$1"
            shift
            ;;
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
        *)
            echo "Unsupported option: $1"
            usage
            exit 1
            ;;
    esac
done

BUILD_DIR="build/$BUILD_TYPE"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: build configuration '$BUILD_TYPE' does not exist at $BUILD_DIR"
    echo "Run ./scripts/ezbuild.sh $BUILD_TYPE first"
    exit 1
fi

cmake --build "$BUILD_DIR" --target EngineUnitTests -- -j$(nproc || echo 8)

if [ "$LIST_TESTS" -eq 1 ]; then
    ctest --test-dir "$BUILD_DIR" -N
    exit 0
fi

if [ -n "$TEST_PATTERN" ] && [ -n "$LABEL" ]; then
    ctest --test-dir "$BUILD_DIR" --output-on-failure -R "$TEST_PATTERN" -L "$LABEL"
elif [ -n "$TEST_PATTERN" ]; then
    ctest --test-dir "$BUILD_DIR" --output-on-failure -R "$TEST_PATTERN"
elif [ -n "$LABEL" ]; then
    ctest --test-dir "$BUILD_DIR" --output-on-failure -L "$LABEL"
else
    ctest --test-dir "$BUILD_DIR" --output-on-failure
fi
