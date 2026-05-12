#!/usr/bin/env sh

# Must be sourced so PATH changes persist in the current shell.
if [ "$0" = "$BASH_SOURCE" ] || { [ -n "$ZSH_VERSION" ] && [ "${(%):-%N}" = "$0" ]; }; then
  echo "ERROR: Please source this script, do not execute it."
  echo "Usage: source scripts/ezprepare.sh"
  return 1 2>/dev/null || exit 1
fi

if [ -n "$BASH_SOURCE" ]; then
  SCRIPT_PATH="$BASH_SOURCE"
elif [ -n "$ZSH_VERSION" ]; then
  SCRIPT_PATH="${(%):-%N}"
else
  SCRIPT_PATH="$0"
fi

# Resolve the real scripts directory from the sourced file instead of the caller's working
# directory; developers usually source this once from arbitrary shells.
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" 2>/dev/null && pwd)"

if [ -z "$SCRIPT_DIR" ]; then
  echo "ERROR: Could not determine scripts directory."
  return 1 2>/dev/null || exit 1
fi

COMMAND_DIR="$SCRIPT_DIR/scripts"
mkdir -p "$COMMAND_DIR"
SYSTEM_NAME="$(uname -s)"

# Rebuild the command directory from the current script layout so moved or deleted scripts do not
# leave stale completions behind.
for old_command in "$COMMAND_DIR"/ez*.sh; do
  [ -e "$old_command" ] || continue
  rm -f "$old_command"
done

windows_path() {
  if command -v cygpath >/dev/null 2>&1; then
    cygpath -w "$1"
  else
    printf "%s" "$1"
  fi
}

make_symlink() {
  target_path=$1
  link_path=$2

  rm -f "$link_path"
  ln -s "$target_path" "$link_path" 2>/dev/null || true

  if [ -L "$link_path" ]; then
    return 0
  fi

  rm -f "$link_path"

  if echo "$SYSTEM_NAME" | grep -E 'MSYS|MINGW|CYGWIN' >/dev/null; then
    link_windows=$(windows_path "$link_path")
    target_windows=$(windows_path "$target_path")
    cmd //c mklink "$link_windows" "$target_windows" >/dev/null 2>&1
  else
    ln -s "$target_path" "$link_path"
  fi
}

make_shim() {
  target_path=$1
  link_path=$2

  # MSYS/MinGW symlinks are privilege-dependent and can behave differently across machines.
  # A tiny executable shim still gives PATH lookup and tab completion without requiring symlinks.
  cat >"$link_path" <<EOF
#!/usr/bin/env sh
exec sh "$target_path" "\$@"
EOF
  chmod +x "$link_path"
}

link_script() {
  source_path=$1
  command_name=$(basename "$source_path")
  link_path="$COMMAND_DIR/$command_name"

  if [ "$source_path" = "$link_path" ]; then
    return 0
  fi

  # Prefer deterministic shims on Windows shells. Non-Windows shells get real symlinks.
  if echo "$SYSTEM_NAME" | grep -E 'MSYS|MINGW|CYGWIN' >/dev/null; then
    rm -f "$link_path"
    make_shim "$source_path" "$link_path"
    echo "Shimmed $command_name -> ${source_path#"$SCRIPT_DIR/"}"
    return 0
  fi

  if ! make_symlink "$source_path" "$link_path"; then
    echo "ERROR: Could not create symbolic link: $link_path"
    return 1
  fi

  if [ ! -L "$link_path" ]; then
    echo "ERROR: Created path is not a symbolic link: $link_path"
    return 1
  fi

  echo "Linked $command_name -> ${source_path#"$SCRIPT_DIR/"}"
}

# Publish root-level and one-level nested ez*.sh commands into scripts/scripts.
for script_file in "$SCRIPT_DIR"/ez*.sh "$SCRIPT_DIR"/*/ez*.sh; do
  [ -f "$script_file" ] || continue

  case "$script_file" in
    "$SCRIPT_DIR/ezprepare.sh"|"$COMMAND_DIR"/*)
      ;;
    *)
      if ! link_script "$script_file"; then
        return 1 2>/dev/null || exit 1
      fi
      ;;
  esac
done

case ":$PATH:" in
  *":$COMMAND_DIR:"*)
    echo "Already in PATH: $COMMAND_DIR"
    ;;
  *)
    export PATH="$COMMAND_DIR:$PATH"
    echo "Added to PATH: $COMMAND_DIR"
    ;;
esac
