../../bin/ANN_test Rothermel.ffann modelrun.csv print | grep result > result.txt
if ! diff result.txt result.txt.ref; then
    echo "ANN results differs from reference."
    exit 1
fi