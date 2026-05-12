#!/usr/bin/env sh

COLOR_RESET="$(printf '\033[0m')"
COLOR_BLUE="$(printf '\033[1;34m')"
COLOR_GREEN="$(printf '\033[1;32m')"
COLOR_YELLOW="$(printf '\033[1;33m')"
COLOR_RED="$(printf '\033[1;31m')"

export CLICOLOR_FORCE=1

# Shared status printer for scripts that want consistent colored progress messages.
print_status() {
    color=$1
    message=$2
    printf "%s==> %s%s\n" "$color" "$message" "$COLOR_RESET"
}
