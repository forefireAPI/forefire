#!/usr/bin/env bash
set -e

# 1. Create (or reuse) the build directory quietly
mkdir -p build

# 2. Configure into build/
cmake -S . -B build

# 3. Build in parallel on all cores
cmake --build build -- -j"$(nproc)"
