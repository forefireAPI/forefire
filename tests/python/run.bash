$PYTHONEXE percolation.py
$PYTHONEXE idealizedwind.py

# Pure-logic checker for the landscape validator; needs no built extension.
$PYTHONEXE test_validate.py || { echo "test_validate.py failed."; exit 1; }

# Basic sanity checks on generated output
# Check that 360wind.png exists and is not empty
if [ ! -s 360wind.png ]; then
    echo "360wind.png is empty or missing."
    exit 1
fi