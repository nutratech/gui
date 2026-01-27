#!/usr/bin/env bash
# Version bump script for semantic versioning with pre-release support
# Usage: ./scripts/version-bump.sh [bump_type] [pre_release_type]
#   bump_type: major|minor|patch (default: patch)
#   pre_release_type: none|alpha|beta|rc (default: none)

set -euo pipefail

BUMP_TYPE="${1:-patch}"
PRE_TYPE="${2:-none}"

# Get latest tag, remove 'v' prefix
LATEST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "v0.0.0")
VERSION=${LATEST_TAG#v}

# Parse current version
BASE_VERSION=$(echo "$VERSION" | cut -d'-' -f1)
PRERELEASE_PART=$(echo "$VERSION" | cut -d'-' -f2- -s)

IFS='.' read -r MAJOR MINOR PATCH <<<"$BASE_VERSION"

# If currently no prerelease part, simple bump logic or start new prerelease
if [ -z "$PRERELEASE_PART" ]; then
	if [ "$BUMP_TYPE" == "major" ]; then
		MAJOR=$((MAJOR + 1))
		MINOR=0
		PATCH=0
	elif [ "$BUMP_TYPE" == "minor" ]; then
		MINOR=$((MINOR + 1))
		PATCH=0
	else
		PATCH=$((PATCH + 1))
	fi

	if [ "$PRE_TYPE" != "none" ]; then
		NEW_TAG="v$MAJOR.$MINOR.$PATCH-$PRE_TYPE.1"
	else
		NEW_TAG="v$MAJOR.$MINOR.$PATCH"
	fi
else
	# Existing prerelease (e.g., 1.0.0-beta.1)
	CURRENT_PRE_TYPE=$(echo "$PRERELEASE_PART" | cut -d'.' -f1)
	CURRENT_PRE_NUM=$(echo "$PRERELEASE_PART" | cut -d'.' -f2)

	if [ "$PRE_TYPE" == "none" ]; then
		# Promotion to stable: 1.0.0-beta.1 -> 1.0.0
		NEW_TAG="v$MAJOR.$MINOR.$PATCH"
	elif [ "$PRE_TYPE" == "$CURRENT_PRE_TYPE" ]; then
		# Increment same prerelease type: 1.0.0-beta.1 -> 1.0.0-beta.2
		NEW_NUM=$((CURRENT_PRE_NUM + 1))
		NEW_TAG="v$MAJOR.$MINOR.$PATCH-$PRE_TYPE.$NEW_NUM"
	else
		# Switching type, restart count: beta.2 -> rc.1
		NEW_TAG="v$MAJOR.$MINOR.$PATCH-$PRE_TYPE.1"
	fi
fi

echo "$NEW_TAG"
