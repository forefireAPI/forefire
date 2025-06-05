$PYTHONEXE percolation.py
$PYTHONEXE idealizedwind.py

# Basic sanity checks on generated output
# Check that 360wind.png exists and is not empty
if [ ! -s 360wind.png ]; then
    echo "360wind.png is empty or missing."
    exit 1
fi