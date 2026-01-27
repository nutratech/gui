#!/usr/bin/env bash
# Version bump script for semantic versioning with pre-release support
# Usage: ./scripts/ci-version-bump.sh [bump_type] [pre_release_type] [--tag] [--push]
#   bump_type: major|minor|patch (default: patch)
#   pre_release_type: none|alpha|beta|rc (default: none)
#   --tag: Create the git tag
#   --push: Push the tag to origin (implies --tag)

set -euo pipefail

BUMP_TYPE="${1:-patch}"
PRE_TYPE="${2:-none}"
DO_TAG=false
DO_PUSH=false

# Parse flags
shift 2 2>/dev/null || true
for arg in "$@"; do
	case $arg in
	--tag) DO_TAG=true ;;
	--push)
		DO_TAG=true
		DO_PUSH=true
		;;
	esac
done

# Get latest tag
LATEST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "v0.0.0")
VERSION=${LATEST_TAG#v}

# Parse current version
BASE_VERSION=$(echo "$VERSION" | cut -d'-' -f1)
PRERELEASE_PART=$(echo "$VERSION" | cut -d'-' -f2- -s)

IFS='.' read -r MAJOR MINOR PATCH <<<"$BASE_VERSION"

# Compute new version
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
	CURRENT_PRE_TYPE=$(echo "$PRERELEASE_PART" | cut -d'.' -f1)
	CURRENT_PRE_NUM=$(echo "$PRERELEASE_PART" | cut -d'.' -f2)

	if [ "$PRE_TYPE" == "none" ]; then
		NEW_TAG="v$MAJOR.$MINOR.$PATCH"
	elif [ "$PRE_TYPE" == "$CURRENT_PRE_TYPE" ]; then
		NEW_NUM=$((CURRENT_PRE_NUM + 1))
		NEW_TAG="v$MAJOR.$MINOR.$PATCH-$PRE_TYPE.$NEW_NUM"
	else
		NEW_TAG="v$MAJOR.$MINOR.$PATCH-$PRE_TYPE.1"
	fi
fi

echo "Bumping from $LATEST_TAG to $NEW_TAG"

if [ "$DO_TAG" = true ]; then
	git tag -a "$NEW_TAG" -m "Release $NEW_TAG"
	echo "Created tag $NEW_TAG"
fi

if [ "$DO_PUSH" = true ]; then
	git push origin "$NEW_TAG"
	echo "Pushed tag $NEW_TAG to origin"
fi

# Output just the tag for scripts that need to capture it
if [ "$DO_TAG" = false ]; then
	echo "$NEW_TAG"
fi
