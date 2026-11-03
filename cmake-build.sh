#!/usr/bin/env bash
# Run under bash even when called as "sh cmake-build.sh".
if [ -z "${BASH_VERSION:-}" ]; then exec bash "$0" "$@"; fi

#export NETCDF_HOME=$HOME/
#export ECCODES_HOME=$HOME/
NPROC="${NPROC:-$(getconf _NPROCESSORS_ONLN)}"

set -e

# 1. Create (or reuse) the build directory quietly
mkdir -p build

# 2. Configure into build/
cmake -S . -B build

# 3. Build in parallel on all cores
cmake --build build -- -j"$NPROC"
