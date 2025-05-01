#!/bin/bash

set -e

GENERATED_KML="real_case.kml"
REFERENCE_KML="real_case.kml.ref"
# GENERATED_NC="ForeFire.0.nc"  # Not checked yet
# REFERENCE_NC="ForeFire.0.nc.ref" # Not checked yet

FOREFIRE_EXE="../../bin/forefire"

echo "Running ForeFire simulation (real_case)..."
$FOREFIRE_EXE -i real_case.ff

# --- Verification ---
echo "Verifying KML output..."

if ! command -v python3 &> /dev/null
then
    echo "Error: python3 command not found. Please install Python 3."
    exit 1
fi

if [ ! -f compare_kml.py ]; then
    echo "Error: compare_kml.py script not found in current directory."
    exit 1
fi

python3 compare_kml.py "$GENERATED_KML" "$REFERENCE_KML"
echo "KML verification passed."

# --- NetCDF Check (Commented Out) ---
# echo "Verifying NetCDF output... (SKIPPED)"
# python3 compare_nc.py "$GENERATED_NC" "$REFERENCE_NC"
# echo "NetCDF verification passed."

echo "Minimal KML test passed for runff."
exit 0