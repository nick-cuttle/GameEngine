#!/usr/bin/env sh

# Must be sourced
if [ "$0" = "$BASH_SOURCE" ] || { [ -n "$ZSH_VERSION" ] && [ "${(%):-%N}" = "$0" ]; }; then
  echo "ERROR: Please source this script, do not execute it."
  echo "Usage: source add_path.sh [path]"
  return 1 2>/dev/null || exit 1
fi

# Determine target path
if [ -n "$1" ]; then
  NEW_PATH="$1"
else
  NEW_PATH="$(cd "$(dirname "${BASH_SOURCE:-$0}")" 2>/dev/null && pwd)"
  [ -z "$NEW_PATH" ] && NEW_PATH="$(pwd)"
fi

NEW_PATH="${NEW_PATH%/}"

# Validate directory existence
if [ ! -d "$NEW_PATH" ]; then
  echo "ERROR: Path does not exist or is not a directory: $NEW_PATH"
  return 1 2>/dev/null || exit 1
fi

# Prevent duplicates
case ":$PATH:" in
  *":$NEW_PATH:"*)
    echo "Already in PATH: $NEW_PATH"
    ;;
  *)
    export PATH="$NEW_PATH:$PATH"
    echo "Added to PATH: $NEW_PATH"
    ;;
esac

echo "Current PATH:"
echo "$PATH"