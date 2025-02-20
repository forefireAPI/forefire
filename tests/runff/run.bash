
# Run simulations
../../bin/forefire -i real_case.ff
../../bin/forefire -i reload_case.ff

# Compare output with reference files
if ! diff real_case.kml real_case.kml.ref; then
    echo "real_case.kml does not match reference."
    exit 1
fi

if ! diff ForeFire.0.nc ForeFire.0.nc.ref; then
    echo "ForeFire.0.nc does not match reference."
    exit 1
fi

exit 0