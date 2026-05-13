#!/usr/bin/env sh

set -e

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"

if [ -f "$SCRIPT_DIR/../build/buildHelper.sh" ]; then
    BUILD_HELPER_PATH="$SCRIPT_DIR/../build/buildHelper.sh"
    . "$SCRIPT_DIR/../build/buildHelper.sh"
else
    BUILD_HELPER_PATH="$SCRIPT_DIR/buildHelper.sh"
    . "$SCRIPT_DIR/buildHelper.sh"
fi

BUILD_DIR="build/Deliver/Release"
DELIVER_ROOT="build/Deliver"
SKIP_BUILD=0
PACKAGE_FORMAT="all"

usage() {
    echo "Usage: ezdeliver.sh [options]"
    echo
    echo "Builds a release install tree and packages it for the current host."
    echo
    echo "Options:"
    echo "  --build-dir <dir>      Build directory to package. Default: build/Deliver/Release"
    echo "  --package-dir <dir>    Delivery output directory. Default: build/Deliver"
    echo "  --format <format>      Archive format: all, tar.gz, zip. Default: all"
    echo "  --skip-build           Package an already-built directory"
    echo "  -h, --help             Show this help"
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --build-dir)
            BUILD_DIR=$2
            shift 2
            ;;
        --package-dir)
            DELIVER_ROOT=$2
            shift 2
            ;;
        --format)
            PACKAGE_FORMAT=$2
            shift 2
            ;;
        --skip-build)
            SKIP_BUILD=1
            shift
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

case "$PACKAGE_FORMAT" in
    all|tar.gz|zip)
        ;;
    *)
        echo "Unsupported package format: $PACKAGE_FORMAT"
        usage
        exit 1
        ;;
esac

cd "$REPO_ROOT"
init_build_helper

BUILD_TYPE=$(infer_build_type_from_dir "$BUILD_DIR") || exit 1
VERSION=$(sed -n 's/.*"version-string"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' vcpkg.json | sed -n '1p')
VERSION=${VERSION:-0.1.0}

SYSTEM_NAME=$(uname -s)
case "$SYSTEM_NAME" in
    Linux*) HOST_OS=linux ;;
    Darwin*) HOST_OS=macos ;;
    MINGW*|MSYS*|CYGWIN*) HOST_OS=windows ;;
    *) HOST_OS=$(printf "%s" "$SYSTEM_NAME" | tr '[:upper:]' '[:lower:]' | tr -c '[:alnum:]' '-') ;;
esac

HOST_ARCH=$(printf "%s" "$(uname -m)" | tr '[:upper:]' '[:lower:]' | tr -c '[:alnum:]' '-')
PACKAGE_NAME="gameengine-$VERSION-$HOST_OS-$HOST_ARCH"
STAGE_ROOT="$DELIVER_ROOT/stage"
PACKAGE_ROOT="$STAGE_ROOT/$PACKAGE_NAME"
PACKAGE_DIR="$DELIVER_ROOT/packages"
CONFIGURE_LOG="$BUILD_DIR/ezdeliver-configure.log"

if [ "$SKIP_BUILD" -eq 0 ]; then
    print_status "$COLOR_BLUE" "Configuring delivery build $BUILD_DIR ($BUILD_TYPE)"
    configure_build "$BUILD_DIR" "$BUILD_TYPE" "$CONFIGURE_LOG" "-DBUILD_TESTS=OFF"

    print_status "$COLOR_BLUE" "Building delivery artifacts"
    build_configured_dir "$BUILD_DIR" ""
else
    print_status "$COLOR_YELLOW" "Skipping build; packaging existing directory $BUILD_DIR"
fi

if [ ! -d "$BUILD_DIR" ]; then
    print_status "$COLOR_RED" "Build directory does not exist: $BUILD_DIR"
    exit 1
fi

rm -rf "$PACKAGE_ROOT"
mkdir -p "$PACKAGE_ROOT" "$PACKAGE_DIR"

print_status "$COLOR_BLUE" "Installing into package stage"
cmake --install "$BUILD_DIR" --config "$BUILD_TYPE" --prefix "$PACKAGE_ROOT"

if [ "$HOST_OS" = "windows" ] && [ -d "$BUILD_DIR/bin" ]; then
    for runtime_dll in "$BUILD_DIR"/bin/*.dll; do
        [ -f "$runtime_dll" ] || continue
        cp "$runtime_dll" "$PACKAGE_ROOT/bin/"
    done
fi

cat >"$PACKAGE_ROOT/install.sh" <<'EOF'
#!/usr/bin/env sh

set -e

PREFIX="$HOME/.local"

usage() {
    echo "Usage: install.sh [--prefix <dir>]"
    echo "Default prefix: $HOME/.local"
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --prefix)
            PREFIX=$2
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

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"

copy_tree() {
    name=$1
    source_dir="$SCRIPT_DIR/$name"
    target_dir="$PREFIX/$name"

    [ -d "$source_dir" ] || return 0
    mkdir -p "$target_dir"
    cp -R "$source_dir/." "$target_dir/"
}

copy_tree bin
copy_tree lib
copy_tree include
copy_tree share

echo "Installed GameEngine to $PREFIX"
echo "Add $PREFIX/bin to PATH if you want to run GameEngine from any shell."
EOF
chmod +x "$PACKAGE_ROOT/install.sh"

cat >"$PACKAGE_ROOT/install.ps1" <<'EOF'
param(
    [string]$Prefix = "$HOME\GameEngine"
)

$ErrorActionPreference = "Stop"
$Source = Split-Path -Parent $MyInvocation.MyCommand.Path

foreach ($Name in @("bin", "lib", "include", "share")) {
    $SourceDir = Join-Path $Source $Name
    if (Test-Path $SourceDir) {
        $TargetDir = Join-Path $Prefix $Name
        New-Item -ItemType Directory -Force -Path $TargetDir | Out-Null
        Copy-Item -Path (Join-Path $SourceDir "*") -Destination $TargetDir -Recurse -Force
    }
}

Write-Host "Installed GameEngine to $Prefix"
Write-Host "Add $(Join-Path $Prefix 'bin') to PATH if you want to run GameEngine from any shell."
EOF

cat >"$PACKAGE_ROOT/MANIFEST.txt" <<EOF
name=GameEngine
version=$VERSION
host_os=$HOST_OS
host_arch=$HOST_ARCH
build_type=$BUILD_TYPE
EOF

create_tar_gz() {
    archive_path="$PACKAGE_DIR/$PACKAGE_NAME.tar.gz"
    rm -f "$archive_path"
    (
        cd "$STAGE_ROOT"
        cmake -E tar czf "../packages/$PACKAGE_NAME.tar.gz" "$PACKAGE_NAME"
    )
    print_status "$COLOR_GREEN" "Created $archive_path"
}

create_zip() {
    archive_path="$PACKAGE_DIR/$PACKAGE_NAME.zip"
    rm -f "$archive_path"
    (
        cd "$STAGE_ROOT"
        cmake -E tar cf "../packages/$PACKAGE_NAME.zip" --format=zip "$PACKAGE_NAME"
    )
    print_status "$COLOR_GREEN" "Created $archive_path"
}

case "$PACKAGE_FORMAT" in
    all)
        create_tar_gz
        create_zip
        ;;
    tar.gz)
        create_tar_gz
        ;;
    zip)
        create_zip
        ;;
esac

print_status "$COLOR_GREEN" "Delivery package staged at $PACKAGE_ROOT"
echo "Install after extracting:"
echo "  sh install.sh --prefix \"\$HOME/.local\""
echo "  powershell -ExecutionPolicy Bypass -File install.ps1 -Prefix \"C:\\GameEngine\""
