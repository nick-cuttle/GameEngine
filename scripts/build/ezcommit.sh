#!/usr/bin/env sh

set -e

if [ -z "$1" ]; then
  echo "Usage: ezcommit.sh \"commit message\""
  exit 1
fi

if [ "$#" -ne 1 ]; then
  echo "Error: Provide exactly one commit message in quotes"
  exit 1
fi

COMMIT_MSG="$1"
BRANCH_NAME=$(git rev-parse --abbrev-ref HEAD)
TAG=$(echo "$BRANCH_NAME" | cut -d '-' -f 1)

if ! echo "$TAG" | grep -E '^[0-9]+$' >/dev/null; then
  echo "Error: Branch name must start with a numeric tag (e.g., 11-feature-name)"
  exit 1
fi

if echo "$COMMIT_MSG" | grep -E "#${TAG}$" >/dev/null; then
  FINAL_MSG="$COMMIT_MSG"
else
  FINAL_MSG="${COMMIT_MSG} #${TAG}"
fi

git commit -m "$FINAL_MSG"
