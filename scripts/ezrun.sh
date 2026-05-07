#!/usr/bin/env sh

# Default version is Release unless a parameter is provided
VERSION="${1:-Release}"

EXE_NAME="GameEngine.exe"
BASE_DIR="build/$VERSION"
BIN_DIR="$BASE_DIR/bin"
EXECUTABLE="$BIN_DIR/$EXE_NAME"

# Validate version directory exists
if [ ! -d "$BASE_DIR" ]; then
    echo "Error: build configuration '$VERSION' does not exist at $BASE_DIR"
    echo "Available options: Release, Debug (or any other built configs)"
    exit 1
fi

# Check executable exists
if [ -f "$EXECUTABLE" ]; then
    echo "Running $VERSION build..."
    echo "Executable: $EXECUTABLE"
    "$EXECUTABLE"
else
    echo "Error: executable not found: $EXECUTABLE"
    exit 1
fi