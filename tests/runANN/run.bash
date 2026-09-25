#!/bin/bash
# Checks that the trained network in Rothermel.ffann still reproduces the
# propagation model it was fitted to.
#
# This used to diff the per-input predictions against result.txt.ref, a file
# that is not in the repository, so the test failed on line 2 every time it
# ran. A plain rename of the committed result.txt would not have fixed it:
# regenerating that output changes a handful of lines in the last digit
# (12412.1 against 12412.2), so an exact diff is red on any machine but the
# one that produced the file.
#
# ANN_test already computes the root mean squared error between what the
# network predicts and what the CSV says the model produced, so that is what
# is checked here. It needs no reference file, it does not care about the last
# digit, and it fails loudly if the network stops approximating the model.
#
# For a per-input dump, add `print`:
#   ../../bin/ANN_test Rothermel.ffann modelrun.csv print

set -e

ANN_TEST=../../bin/ANN_test
NETWORK=Rothermel.ffann
INPUTS=modelrun.csv

# Calibration, measured on this fixture:
#
#   trained network                     0.0235
#   predicting the mean of every output 0.0966
#   normalisation weights scaled by 1%  17817
#
# So the tolerance has to sit between the first two, or a network that ignores
# its inputs entirely would pass. 0.05 leaves the real network a factor of two
# of headroom and still fails a constant predictor. Anything that actually
# breaks the arithmetic lands orders of magnitude away and is not close.
#
# Note the fixture is weak: the expected outputs take four distinct values
# spanning 1.1 in 12412. That is why the gap between a working network and a
# constant one is so narrow, and it is worth replacing with a set of inputs
# that produce a real spread of rates of spread.
MAX_RMSE=0.05

if [ ! -x "$ANN_TEST" ]; then
    echo "ANN_test not found at $ANN_TEST. Build with -DFOREFIRE_BUILD_TOOLS=ON."
    exit 1
fi
for required in "$NETWORK" "$INPUTS"; do
    if [ ! -s "$required" ]; then
        echo "missing input file: $required"
        exit 1
    fi
done

output=$("$ANN_TEST" "$NETWORK" "$INPUTS")
echo "$output"

rmse=$(printf '%s\n' "$output" | sed -n 's/^Total Root Mean Squared Error: //p')
processed=$(printf '%s\n' "$output" | sed -n 's/^Time taken for processing \([0-9]*\) inputs.*/\1/p')

if [ -z "$rmse" ]; then
    echo "ANN_test printed no RMSE; it did not run to completion."
    exit 1
fi

if [ -z "$processed" ] || [ "$processed" -eq 0 ]; then
    echo "ANN_test processed no inputs."
    exit 1
fi

if ! awk -v got="$rmse" -v max="$MAX_RMSE" 'BEGIN { exit !(got <= max) }'; then
    echo "ANN RMSE $rmse exceeds the tolerance of $max."
    echo "The network no longer reproduces the propagation model it was fitted to."
    exit 1
fi

echo "ANN test passed: $processed inputs, RMSE $rmse (tolerance $MAX_RMSE)."
