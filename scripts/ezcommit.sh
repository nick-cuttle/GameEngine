#!/usr/bin/env sh

set -e

# Ensure a commit message is provided
if [ -z "$1" ]; then
  echo "Usage: ./ezcommit.sh \"commit message\""
  exit 1
fi

if [ "$#" -ne 1 ]; then
  echo "Error: Provide exactly one commit message in quotes"
  exit 1
fi

COMMIT_MSG="$1"

# Get current branch name
BRANCH_NAME=$(git rev-parse --abbrev-ref HEAD)

# Extract numeric tag (everything before first '-')
TAG=$(echo "$BRANCH_NAME" | cut -d '-' -f 1)

# Validate tag is numeric
if ! [[ "$TAG" =~ ^[0-9]+$ ]]; then
  echo "Error: Branch name must start with a numeric tag (e.g., 11-feature-name)"
  exit 1
fi

# Check if message already ends with #TAG
if [[ "$COMMIT_MSG" =~ \#${TAG}$ ]]; then
  FINAL_MSG="$COMMIT_MSG"
else
  FINAL_MSG="${COMMIT_MSG} #${TAG}"
fi

# Perform commit
git commit -m "$FINAL_MSG"
