#!/usr/bin/env sh

set -e

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"

if [ -f "$SCRIPT_DIR/../core/printHelper.sh" ]; then
    . "$SCRIPT_DIR/../core/printHelper.sh"
else
    . "$SCRIPT_DIR/printHelper.sh"
fi

SYSTEM_NAME=$(uname -s)
case "$SYSTEM_NAME" in
    Linux*) HOST_OS=linux ;;
    Darwin*) HOST_OS=macos ;;
    MINGW*|MSYS*|CYGWIN*) HOST_OS=windows ;;
    *) HOST_OS=$(printf "%s" "$SYSTEM_NAME" | tr '[:upper:]' '[:lower:]' | tr -c '[:alnum:]' '-') ;;
esac

case "$HOST_OS" in
    windows) DEFAULT_PREFIX="$HOME/GameEngine" ;;
    *) DEFAULT_PREFIX="$HOME/.local" ;;
esac

VERSION=$(sed -n 's/.*"version-string"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "$REPO_ROOT/vcpkg.json" | sed -n '1p')
VERSION=${VERSION:-0.1.0}
HOST_ARCH=$(printf "%s" "$(uname -m)" | tr '[:upper:]' '[:lower:]' | tr -c '[:alnum:]' '-')

PREFIX=$DEFAULT_PREFIX
PACKAGE_DIR="$REPO_ROOT/build/Deliver/stage/gameengine-$VERSION-$HOST_OS-$HOST_ARCH"

usage() {
    echo "Usage: ezinstall.sh [options]"
    echo
    echo "Installs a staged GameEngine delivery package."
    echo
    echo "Options:"
    echo "  --prefix <dir>     Install destination. Default: $DEFAULT_PREFIX"
    echo "  --package <dir>    Staged package directory. Default: current host delivery stage"
    echo "  -h, --help         Show this help"
    echo
    echo "Examples:"
    echo "  ezinstall.sh"
    echo "  ezinstall.sh --prefix \"\$HOME/.local\""
    echo "  ezinstall.sh --prefix \"C:/GameEngine\""
    echo "  ezinstall.sh --package build/Deliver/stage/gameengine-0.1.0-windows-x86-64"
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --prefix)
            PREFIX=$2
            shift 2
            ;;
        --package)
            PACKAGE_DIR=$2
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

case "$PACKAGE_DIR" in
    /*|[A-Za-z]:/*|[A-Za-z]:\\*) ;;
    *) PACKAGE_DIR="$REPO_ROOT/$PACKAGE_DIR" ;;
esac

if [ ! -d "$PACKAGE_DIR" ]; then
    print_status "$COLOR_RED" "Package directory does not exist: $PACKAGE_DIR"
    echo "Run ezdeliver.sh first, or pass --package <dir> for an extracted/staged package."
    exit 1
fi

copy_tree() {
    name=$1
    source_dir="$PACKAGE_DIR/$name"
    target_dir="$PREFIX/$name"

    [ -d "$source_dir" ] || return 0
    mkdir -p "$target_dir"
    cp -R "$source_dir/." "$target_dir/"
}

print_status "$COLOR_BLUE" "Installing GameEngine from $PACKAGE_DIR"
copy_tree bin
copy_tree lib
copy_tree include
copy_tree share

print_status "$COLOR_GREEN" "Installed GameEngine to $PREFIX"
echo "Add this directory to PATH if you want to run GameEngine from any shell:"
echo "  $PREFIX/bin"
