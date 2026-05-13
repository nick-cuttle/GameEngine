#!/usr/bin/env sh

# Default build directory is build/Release unless a parameter is provided
BASE_DIR="${1:-build/Release}"

BIN_DIR="$BASE_DIR/bin"
EXECUTABLE="$BIN_DIR/GameEngine"

if [ ! -f "$EXECUTABLE" ]; then
    EXECUTABLE="$BIN_DIR/GameEngine.exe"
fi

# Validate build directory exists
if [ ! -d "$BASE_DIR" ]; then
    echo "Error: build directory does not exist: $BASE_DIR"
    echo "Run ezbuild.sh $BASE_DIR first"
    exit 1
fi

if [ -f "$EXECUTABLE" ]; then
    echo "Running build from $BASE_DIR..."
    echo "Executable: $EXECUTABLE"
    "$EXECUTABLE"
else
    echo "Error: executable not found in $BIN_DIR"
    echo "Expected GameEngine or GameEngine.exe"
    exit 1
fi
