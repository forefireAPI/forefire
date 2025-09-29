#!/usr/bin/env bash
set -euo pipefail

# Minimal preflight check for Git LFS assets used by quick-start tests
# Verifies that tests/runff/data.nc is a real NetCDF file (by size) and, if available, via ncdump.

PROJECT_ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")"/../.. && pwd)"
DATA_FILE_REL="tests/runff/data.nc"
DATA_FILE="${PROJECT_ROOT_DIR}/${DATA_FILE_REL}"

if [[ ! -f "${DATA_FILE}" ]]; then
  echo "ERROR: ${DATA_FILE_REL} not found. Did you fetch Git LFS files?"
  echo "Hint: Install Git LFS and reclone, or download the file from GitHub UI into ${DATA_FILE_REL}."
  exit 1
fi

# Cross-platform file size (Linux: stat -c, macOS: stat -f)
if size_bytes=$(stat -c%s "${DATA_FILE}" 2>/dev/null); then
  :
elif size_bytes=$(stat -f%z "${DATA_FILE}" 2>/dev/null); then
  :
else
  echo "WARNING: Could not determine file size via stat. Skipping size check."
  size_bytes=0
fi

echo "Found ${DATA_FILE_REL} (size: ${size_bytes} bytes)"

# Threshold: > 1 MB indicates not an LFS pointer
MIN_BYTES=1000000
if [[ "${size_bytes}" -lt "${MIN_BYTES}" ]]; then
  echo "ERROR: ${DATA_FILE_REL} appears to be a Git LFS pointer (size ${size_bytes} bytes)."
  echo "Fix: Install Git LFS and reclone the repository, or use 'git lfs fetch --all && git lfs checkout'."
  exit 1
fi

if command -v ncdump >/dev/null 2>&1; then
  if ! ncdump -k "${DATA_FILE}" >/dev/null 2>&1; then
    echo "ERROR: ncdump failed to read ${DATA_FILE_REL}. The file may be corrupted."
    exit 1
  fi
fi

echo "OK: ${DATA_FILE_REL} looks valid."

