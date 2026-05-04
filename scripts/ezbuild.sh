#!/bin/bash

build_type=$1

# Choose generator based on OS
if uname -s | grep Linux >/dev/null; then
    generator="Unix Makefiles"
elif uname -s | grep -E 'MSYS|MINGW' >/dev/null; then
    generator="MinGW Makefiles"
else
    echo "Unsupported operating system!"
    exit 1
fi

# Build function
build() {
    type=$1
    echo "Building $type..."

    # Configure and build with CMake
    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G "$generator" -S . -B build/$type -DCMAKE_BUILD_TYPE=$type
    if [ $? -ne 0 ]; then
        echo "CMake configuration failed for $type"
        exit 1
    fi

    # Build the project
    cmake --build build/$type -- -j$(nproc || echo 8)
    if [ $? -ne 0 ]; then
        echo "CMake build failed for $type"
        exit 1
    fi
}

# Run builds
case "$build_type" in
    Debug)
        build Debug
        ;;
    Release)
        build Release
        ;;
    "")
        build Debug
        build Release
        ;;
    *)
        echo "Unsupported build type, must be Debug, Release, or blank"
        exit 1
        ;;
esac
