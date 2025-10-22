#!/usr/bin/env bash
# check_all_lfs_universal.sh
# Cross-platform Git-LFS integrity verifier (macOS, Linux, WSL, Cygwin)

set -e
set -o pipefail

BASE_URL="https://github.com/forefireAPI/forefire/raw/refs/heads/master"

# File list: md5:path
FILES_MD5_LIST="
a4e76c91af4b72d2770759fce508b640:tests/mnh_ideal/ForeFire/ForeFire.0.nc.ref
afcd2476e600d4d7580bca442b961191:tests/mnh_ideal/RELIEF3D.des
53f1165f78ad3a697e8d12eee8af2b44:tests/mnh_ideal/RELIEF3D.lfi
4e98a1093da9167e545df07a6693582c:tests/mnh_ideal/RELIEF3DX.lfi
92d4a0008e01e2cca58468bd8aa0da37:tests/mnh_real_nested/ForeFire/ForeFire.0.nc.ref
98d167f873fc70a238c2d33c1da9a114:tests/mnh_real_nested/ForeFire/data.nc
a397d61ad1dd4f73a69e002d6e452541:tests/mnh_real_nested/M2.20170617.00.15.des
4df6c56bc7012d41da26d26add903095:tests/mnh_real_nested/M2.20170617.00.15.nc
da97d0c3dd63f6f6d2d941524c526e75:tests/mnh_real_nested/PGD_D3200mA.nested.nc
b64f537ae25fad1d002085cc48a6e5d4:tests/mnh_real_nested/PGD_D800mA.nested.nc
dbe0a29d7f94fc8cbe55470fecf27cc4:tests/mnh_real_nested/RUN12.1.PRUN1.004.des
cd6cf1c61b5433b425728b0c10a6870f:tests/mnh_real_nested/RUN12.1.PRUN1.004.nc
fe3a89db4253de28e9c42cea22ec91cc:tests/python/360wind.png.ref
cb329bea2ea963eccb0815c0471d7bba:tests/runff/ForeFire.0.nc.ref
f66abd2723db31ecaa8a56662c49d972:tests/runff/data.nc
e6d18d2fe17d41bb0cb702678749e623:tests/runff/real_case.kml.ref
"

# Cross-platform md5 utility
calc_md5() {
    local file="$1"
    if command -v md5sum >/dev/null 2>&1; then
        md5sum "$file" | awk '{print $1}'
    elif command -v md5 >/dev/null 2>&1; then
        md5 -q "$file"
    elif command -v openssl >/dev/null 2>&1; then
        openssl md5 "$file" | awk '{print $2}'
    else
        echo "ERROR: no md5 utility found" >&2
        return 1
    fi
}

download_file() {
    local file="$1"
    mkdir -p "$(dirname "$file")"
    curl -fL --retry 3 --retry-delay 2 -o "$file" "$BASE_URL/$file?download=" >/dev/null 2>&1
}

total=0; verified=0; restored=0; failed=0

echo "Checking Git-LFS files integrity ($(date))"
echo "Repository root: $(pwd)"
echo

IFS=$'\n'
for entry in $FILES_MD5_LIST; do
    [ -z "$entry" ] && continue
    total=$((total+1))
    md5_expected="${entry%%:*}"
    file="${entry#*:}"

    printf "%-65s " "$file"

    if [ -f "$file" ]; then
        md5_actual=$(calc_md5 "$file" || echo "")
    else
        md5_actual=""
    fi

    if [ "$md5_actual" = "$md5_expected" ]; then
        echo "OK"
        verified=$((verified+1))
        continue
    fi

    echo "MISSING/CORRUPTED → downloading..."
    if download_file "$file"; then
        md5_actual=$(calc_md5 "$file" || echo "")
        if [ "$md5_actual" = "$md5_expected" ]; then
            echo "  Restored OK"
            restored=$((restored+1))
        else
            echo "  FAILED (checksum mismatch)"
            failed=$((failed+1))
        fi
    else
        echo "  ERROR downloading"
        failed=$((failed+1))
    fi
done

echo
echo "Summary:"
echo "  Total checked ........: $total"
echo "  Verified OK ..........: $verified"
echo "  Restored successfully : $restored"
echo "  Failed ...............: $failed"
echo

[ "$failed" -eq 0 ] && exit 0 || exit 2
