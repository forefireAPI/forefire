$PYTHONEXE percolation.py
$PYTHONEXE idealized_wind.py
if ! diff 360wind.png 360wind.png.ref; then
    echo "Python: image plot different"
    exit 1
fi
