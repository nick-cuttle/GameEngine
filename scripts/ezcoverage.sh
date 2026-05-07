#!/usr/bin/env sh

set -e

BUILD_TYPE=""
COVERAGE_BUILD_ROOT="build/Coverage"
COVERAGE_DIR="coverage"
TEST_PATTERN=""
LABEL=""
OPEN_REPORT=0
COLOR_RESET="$(printf '\033[0m')"
COLOR_BLUE="$(printf '\033[1;34m')"
COLOR_GREEN="$(printf '\033[1;32m')"

export CLICOLOR_FORCE=1
export CTEST_COLOR_DIAGNOSTICS=1

usage() {
    echo "Usage: ./scripts/ezcoverage.sh [Debug|Release] [options]"
    echo "With no build type, runs Debug first, then Release, and combines the report."
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
        --open)
            OPEN_REPORT=1
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

if ! command -v gcovr >/dev/null 2>&1; then
    echo "Error: gcovr is required but was not found on PATH."
    echo "Install it with: python -m pip install gcovr"
    exit 1
fi

if uname -s | grep Linux >/dev/null; then
    GENERATOR="Unix Makefiles"
elif uname -s | grep -E 'MSYS|MINGW' >/dev/null; then
    GENERATOR="MinGW Makefiles"
else
    echo "Unsupported operating system!"
    exit 1
fi

JOBS="$(nproc 2>/dev/null || echo 8)"
GCOVR_OBJECT_ARGS=""

run_coverage() {
    BUILD_TYPE="$1"
    BUILD_DIR="$COVERAGE_BUILD_ROOT/$BUILD_TYPE"

    printf "%s==> Configuring %s coverage build%s\n" "$COLOR_BLUE" "$BUILD_TYPE" "$COLOR_RESET"
    cmake \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_CXX_FLAGS="--coverage" \
        -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
        -DCMAKE_SHARED_LINKER_FLAGS="--coverage" \
        -DBUILD_TESTS=ON \
        -G "$GENERATOR" \
        -S . \
        -B "$BUILD_DIR"

    printf "%s==> Building %s coverage unit tests%s\n" "$COLOR_BLUE" "$BUILD_TYPE" "$COLOR_RESET"
    cmake --build "$BUILD_DIR" --target EngineUnitTests -- -j"$JOBS"

    find "$BUILD_DIR" -name '*.gcda' -delete

    printf "%s==> Running %s coverage tests%s\n" "$COLOR_BLUE" "$BUILD_TYPE" "$COLOR_RESET"
    if [ -n "$TEST_PATTERN" ] && [ -n "$LABEL" ]; then
        ctest --test-dir "$BUILD_DIR" --output-on-failure -R "$TEST_PATTERN" -L "$LABEL"
    elif [ -n "$TEST_PATTERN" ]; then
        ctest --test-dir "$BUILD_DIR" --output-on-failure -R "$TEST_PATTERN"
    elif [ -n "$LABEL" ]; then
        ctest --test-dir "$BUILD_DIR" --output-on-failure -L "$LABEL"
    else
        ctest --test-dir "$BUILD_DIR" --output-on-failure
    fi

    GCOVR_OBJECT_ARGS="$GCOVR_OBJECT_ARGS --object-directory $BUILD_DIR"
}

if [ -n "$BUILD_TYPE" ]; then
    run_coverage "$BUILD_TYPE"
else
    run_coverage Debug
    run_coverage Release
fi

mkdir -p "$COVERAGE_DIR"

printf "%s==> Generating coverage reports%s\n" "$COLOR_BLUE" "$COLOR_RESET"
# shellcheck disable=SC2086
gcovr \
    --root . \
    $GCOVR_OBJECT_ARGS \
    --filter "src/" \
    --exclude "tests/" \
    --txt \
    --html-details "$COVERAGE_DIR/index.html" \
    --cobertura "$COVERAGE_DIR/coverage.xml"

printf "%s==> Coverage report written to %s/index.html%s\n" "$COLOR_GREEN" "$COVERAGE_DIR" "$COLOR_RESET"

if [ "$OPEN_REPORT" -eq 1 ]; then
    if command -v xdg-open >/dev/null 2>&1; then
        xdg-open "$COVERAGE_DIR/index.html"
    elif command -v start >/dev/null 2>&1; then
        start "$COVERAGE_DIR/index.html"
    else
        echo "Could not find a command to open the report automatically."
    fi
fi
