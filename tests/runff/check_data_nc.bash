#!/usr/bin/env bash
# check_data_nc.bash
# Vérifie le hash MD5 du fichier data.nc et le retélécharge si nécessaire

set -euo pipefail

EXPECTED_MD5="f66abd2723db31ecaa8a56662c49d972"
FILE="data.nc"
URL="https://github.com/forefireAPI/forefire/raw/refs/heads/master/tests/runff/data.nc?download="

check_md5() {
    [[ -f "$FILE" ]] || return 1
    local md5
    md5=$(md5sum "$FILE" | awk '{print $1}')
    [[ "$md5" == "$EXPECTED_MD5" ]]
}

echo "Checking integrity of $FILE ..."

if check_md5; then
    echo "OK: $FILE checksum verified."
else
    echo "Corrupted data, likely issues with git LFS, downloading..."
    curl -L -o "$FILE" "$URL"
    echo "Download complete, rechecking..."
    if check_md5; then
        echo "OK: $FILE successfully restored."
    else
        echo "ERROR: $FILE still corrupted after download." >&2
        exit 1
    fi
fi
