#!/bin/bash

# Manual version management script
# Usage: ./version_bump.sh [major|minor|patch] [changelog_entry]

INO_FILE="m5lights_v1.ino"
VERSION_TYPE=${1:-patch}
CHANGELOG_ENTRY="$2"

if [ ! -f "$INO_FILE" ]; then
    echo "Error: $INO_FILE not found"
    exit 1
fi

# Extract current version
CURRENT_VERSION=$(grep -o '@version [0-9]\+\.[0-9]\+\.[0-9]\+' "$INO_FILE" | cut -d' ' -f2)

if [ -z "$CURRENT_VERSION" ]; then
    echo "Error: Could not find version in $INO_FILE"
    exit 1
fi

echo "Current version: $CURRENT_VERSION"

# Split version into parts
IFS='.' read -r MAJOR MINOR PATCH <<< "$CURRENT_VERSION"

# Increment based on type
case "$VERSION_TYPE" in
    major)
        MAJOR=$((MAJOR + 1))
        MINOR=0
        PATCH=0
        ;;
    minor)
        MINOR=$((MINOR + 1))
        PATCH=0
        ;;
    patch)
        PATCH=$((PATCH + 1))
        ;;
    *)
        echo "Error: Version type must be major, minor, or patch"
        exit 1
        ;;
esac

NEW_VERSION="$MAJOR.$MINOR.$PATCH"
CURRENT_DATE=$(date +%Y-%m-%d)

echo "New version: $NEW_VERSION"

# Update version in .ino file
sed -i.bak "s/@version [0-9]\+\.[0-9]\+\.[0-9]\+/@version $NEW_VERSION/" "$INO_FILE"
sed -i.bak "s/@date [0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\}/@date $CURRENT_DATE/" "$INO_FILE"
sed -i.bak "s/#define VERSION \"[0-9]\+\.[0-9]\+\.[0-9]\+\"/#define VERSION \"$NEW_VERSION\"/" "$INO_FILE"
sed -i.bak "s/#define BUILD_DATE \"[0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\}\"/#define BUILD_DATE \"$CURRENT_DATE\"/" "$INO_FILE"

# Add changelog entry if provided
if [ -n "$CHANGELOG_ENTRY" ]; then
    # Insert new changelog entry after the last version entry
    sed -i.bak "//// @changelog/a\\
/// v$NEW_VERSION ($CURRENT_DATE) - $CHANGELOG_ENTRY" "$INO_FILE"
fi

# Remove backup files
rm -f "$INO_FILE.bak"

echo "Version updated to $NEW_VERSION"

if [ -n "$CHANGELOG_ENTRY" ]; then
    echo "Changelog entry added: $CHANGELOG_ENTRY"
fi

echo ""
echo "To commit these changes:"
echo "git add $INO_FILE"
echo "git commit -m \"Version bump to $NEW_VERSION: $CHANGELOG_ENTRY\""