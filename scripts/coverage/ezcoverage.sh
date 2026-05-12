#!/usr/bin/env sh

set -e

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
. "$SCRIPT_DIR/../build/buildHelper.sh"
. "$SCRIPT_DIR/../test/testHelper.sh"

BUILD_DIR=""
COVERAGE_BUILD_ROOT="build/Coverage"
COVERAGE_DIR="coverage"
TEST_PATTERN=""
LABEL=""
OPEN_REPORT=0

export CTEST_COLOR_DIAGNOSTICS=1

usage() {
    echo "Usage: ezcoverage.sh [build-dir] [options]"
    echo "Example: ezcoverage.sh build/Coverage/Debug"
    echo "With no build directory, runs build/Coverage/Debug first, then build/Coverage/Release, and combines the report."
    echo "The CMake build type is inferred from Debug or Release in the build directory name."
    echo ""
    echo "Builds the tests with GCC coverage instrumentation, runs them with CTest,"
    echo "and generates text plus HTML coverage reports with gcovr."
    echo ""
    echo "Options:"
    echo "  --test <regex>    Run tests matching a CTest regex"
    echo "  --label <regex>   Run tests with matching CTest labels"
    echo "  --open            Open the HTML report when finished"
    echo "  --help            Show this help"
}

# Parse parameters
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
        --open)
            OPEN_REPORT=1
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

# Check gcovr installed before doing any work
if ! command -v gcovr >/dev/null 2>&1; then
    echo "Error: gcovr is required but was not found on PATH."
    echo "Install it with: python -m pip install gcovr"
    exit 1
fi

init_build_helper

GCOVR_OBJECT_ARGS=""

run_coverage() {

    # infer build type from directory name
    BUILD_DIR="$1"
    BUILD_TYPE=$(infer_build_type_from_dir "$BUILD_DIR") || exit 1
    CONFIGURE_LOG="$BUILD_DIR/ezcoverage-configure.log"
    # cmake flags
    EXTRA_CMAKE_ARGS="-DCMAKE_CXX_FLAGS=--coverage -DCMAKE_EXE_LINKER_FLAGS=--coverage -DCMAKE_SHARED_LINKER_FLAGS=--coverage -DBUILD_TESTS=ON"

    # configure build
    print_status "$COLOR_BLUE" "Configuring $BUILD_DIR coverage build ($BUILD_TYPE)"
    if ! configure_build "$BUILD_DIR" "$BUILD_TYPE" "$CONFIGURE_LOG" "$EXTRA_CMAKE_ARGS"; then
        print_status "$COLOR_RED" "CMake configuration failed for $BUILD_DIR. Check the log for details: $CONFIGURE_LOG"
        exit 1
    fi

    # Remove old counters after building and before running tests so gcovr reports only this run.
    find "$BUILD_DIR" -name '*.gcda' -delete

    # build and run tests
    print_status "$COLOR_BLUE" "Running $BUILD_DIR coverage tests"
    run_eztest "$BUILD_DIR" "$TEST_PATTERN" "$LABEL"

    GCOVR_OBJECT_ARGS="$GCOVR_OBJECT_ARGS --object-directory $BUILD_DIR"
}

if [ -n "$BUILD_DIR" ]; then
    run_coverage "$BUILD_DIR"
else
    # default to Debug and Release if build dir not specified.
    run_coverage "$COVERAGE_BUILD_ROOT/Debug"
    run_coverage "$COVERAGE_BUILD_ROOT/Release"
fi

mkdir -p "$COVERAGE_DIR"

# Run report with gcovr
print_status "$COLOR_BLUE" "Generating coverage reports"
# shellcheck disable=SC2086
gcovr \
    --root . \
    $GCOVR_OBJECT_ARGS \
    --filter "src/" \
    --exclude "tests/" \
    --txt \
    --html-details "$COVERAGE_DIR/index.html" \
    --cobertura "$COVERAGE_DIR/coverage.xml"

print_status "$COLOR_GREEN" "Coverage report written to $COVERAGE_DIR/index.html"

# Open report
if [ "$OPEN_REPORT" -eq 1 ]; then
    if command -v xdg-open >/dev/null 2>&1; then
        xdg-open "$COVERAGE_DIR/index.html"
    elif command -v start >/dev/null 2>&1; then
        start "$COVERAGE_DIR/index.html"
    else
        echo "Could not find a command to open the report automatically."
    fi
fi
