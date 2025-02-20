#!/bin/bash
set -e

# Arrays to hold test results
passed_tests=()
failed_tests=()

# Function to run a test in a given directory and log the result
run_test() {
    local test_name="$1"
    local test_dir="$2"
    if [ -d "$test_dir" ]; then
        echo "Running test in $test_dir..."
        # Run the test. If it fails, record failure.
        if (cd "$test_dir" && ./run.bash); then
            echo "$test_name passed."
            passed_tests+=("$test_name")
        else
            echo "$test_name failed."
            failed_tests+=("$test_name")
        fi
    else
        echo "Directory $test_dir not found; skipping $test_name."
    fi
}

# Run mnh_* tests if SRC_MESONH is set
if [ -z "$SRC_MESONH" ]; then
    echo "SRC_MESONH not set: skipping mnh_* tests."
else
    run_test "mnh_ideal" "mnh_ideal"
    run_test "mnh_real_nested" "mnh_real_nested"
fi

# Run python tests if PYTHONEXE is set
if [ -z "$PYTHONEXE" ]; then
    echo "PYTHONEXE not set: skipping python tests."
else
    run_test "python" "python"
fi

# Run additional tests (e.g., runff and runANN)
run_test "runff" "runff"
run_test "runANN" "runff"  # adjust the directory if runANN is in a different location

# Final summary
echo "--------------------------"
echo "Test Summary:"
echo "Passed tests:"
for test in "${passed_tests[@]}"; do
    echo " - $test"
done

if [ ${#failed_tests[@]} -gt 0 ]; then
    echo "Failed tests:"
    for test in "${failed_tests[@]}"; do
        echo " - $test"
    done
    exit 1
else
    echo "All tests passed."
    exit 0
fi