# Run simulations
../../bin/forefire -i real_case.ff
../../bin/forefire -i reload_case.ff
../../bin/forefire -i rungeojson.ff

# Basic sanity checks on generated output
# Check that real_case.kml exists and is not empty
if [ ! -s real_case.kml ]; then
    echo "real_case.kml is empty or missing."
    exit 1
fi

# Basic sanity checks on generated output
# Check that temp2.geojson exists and is not empty
if [ ! -s temp2.geojson ]; then
    echo "temp2.geojson is empty or missing."
    exit 1
fi


# Check that ForeFire.0.nc exists and is larger than a minimal size
min_size=60000   # bytes; adjust as appropriate for your dataset
if [ ! -s ForeFire.0.nc ]; then
    echo "ForeFire.0.nc is missing."
    exit 1
fi
# Get file size in bytes (portable across macOS and Linux)
actual_size=$(wc -c < ForeFire.0.nc | tr -d '[:space:]')
if [ "$actual_size" -lt "$min_size" ]; then
    echo "ForeFire.0.nc is too small (${actual_size} bytes; expected > ${min_size})."
    exit 1
fi

exit 0