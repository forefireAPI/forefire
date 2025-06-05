../../bin/ANN_test Rothermel.ffann modelrun.csv print | grep result > result.txt
if ! diff result.txt result.txt.ref; then
    echo "ANN results differs from reference."
    exit 1
fi
# Basic sanity checks on generated output
# Check that esult.txt exists and is not empty
if [ ! -s result.txt ]; then
    echo "result.txt is empty or missing."
    exit 1
fi