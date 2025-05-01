#!/bin/bash

set -e

GENERATED_KML="real_case.kml"
REFERENCE_KML="real_case.kml.ref"
GENERATED_NC="ForeFire.0.nc"
REFERENCE_NC="ForeFire.0.nc.ref"

# --- Simulation ---
FOREFIRE_EXE="../../bin/forefire"

echo "Running ForeFire simulation (real_case)..."
$FOREFIRE_EXE -i real_case.ff

# --- Verification ---
echo "Verifying KML output..."
# Make sure python3 is available and comparison script exists
if ! command -v python3 &> /dev/null; then echo "Error: python3 required."; exit 1; fi
if [ ! -f compare_kml.py ]; then echo "Error: compare_kml.py not found."; exit 1; fi

python3 compare_kml.py "$GENERATED_KML" "$REFERENCE_KML"
echo "KML verification passed."

echo "Verifying NetCDF output..."
if [ ! -f compare_nc.py ]; then echo "Error: compare_nc.py not found."; exit 1; fi
# Check if the expected output NetCDF file was actually created
if [ ! -f "$GENERATED_NC" ]; then echo "Error: Expected NetCDF output file '$GENERATED_NC' not found."; exit 1; fi

python3 compare_nc.py "$GENERATED_NC" "$REFERENCE_NC"
echo "NetCDF verification passed."

echo "All KML and NetCDF verifications passed for runff test."
exit 0