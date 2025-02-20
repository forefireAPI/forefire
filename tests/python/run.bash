$PYTHONEXE percolation.py
$PYTHONEXE idealized_wind.py
if ! diff percolation.nc percolation.nc.ref; then
    echo "ANN results differs from reference."
    exit 1
fi
if ! diff 360wind.png 360wind.png.ref; then
    echo "Python: image plot different"
    exit 1
fi
