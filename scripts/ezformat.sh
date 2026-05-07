#!/usr/bin/env bash

find . \
    -path ./build -prune -o \
    -path ./.git -prune -o \
    \( -name "*.cpp" -o -name "*.hpp" -o -name "*.cc" -o -name "*.cxx" -o -name "*.h" \) \
    -exec clang-format -i {} +
