#!/bin/bash

set -e

GENERATED_KML="real_case.kml"
REFERENCE_KML="real_case.kml.ref"
GENERATED_NC="ForeFire.0.nc"
REFERENCE_NC="ForeFire.0.nc.ref"
GENERATED_RELOAD="to_reload.ff"

# --- Cleanup previous run artifacts ---
echo "Cleaning up previous test artifacts..."
rm -f "$GENERATED_KML" "$GENERATED_NC" "$GENERATED_RELOAD"

# --- Simulation ---
FOREFIRE_EXE="../../bin/forefire"

echo ""
echo "Running ForeFire simulation (real_case)..."
$FOREFIRE_EXE -i real_case.ff

echo "Running ForeFire simulation (reload_case)..."
if [ ! -f "$GENERATED_RELOAD" ]; then
    echo "Error: State file '$GENERATED_RELOAD' needed for second run was not created."
    exit 1
fi
$FOREFIRE_EXE -i reload_case.ff # <<<--- ADDED SECOND RUN

# --- Verification ---
echo ""
echo "Verifying KML output..."
# Make sure python3 is available and comparison script exists
if ! command -v python3 &> /dev/null; then echo "Error: python3 required."; exit 1; fi
if [ ! -f compare_kml.py ]; then echo "Error: compare_kml.py not found."; exit 1; fi

python3 compare_kml.py "$GENERATED_KML" "$REFERENCE_KML"
echo "KML verification passed."

echo "Verifying NetCDF output..."
if [ ! -f compare_nc.py ]; then echo "Error: compare_nc.py not found."; exit 1; fi
# Check if the NetCDF file was created (likely by the first run)
if [ ! -f "$GENERATED_NC" ]; then echo "Error: Expected NetCDF output file '$GENERATED_NC' not found."; exit 1; fi

python3 compare_nc.py "$GENERATED_NC" "$REFERENCE_NC"
echo "NetCDF verification passed."

echo "All KML and NetCDF verifications passed for runff test."
exit 0