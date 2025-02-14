#!/usr/bin/env bash
FILE="./src/include/Version.h"

# Extract current version (e.g., "v2.1.25")
current=$(grep 'ff_version' "$FILE" | sed -E 's/.*"(v[0-9]+\.[0-9]+\.[0-9]+)".*/\1/')
if [ -z "$current" ]; then
    echo "Error: Version string not found in $FILE" >&2
    exit 1
fi

# Remove leading "v" and split into major, minor, and patch
IFS='.' read -r maj min pat <<< "${current#v}"

# Increment patch and build new version string
new="v$maj.$min.$((pat+1))"

# Replace the version in the file
sed -i "s/$current/$new/" "$FILE"
echo "Version updated: $current -> $new"
