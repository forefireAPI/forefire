# Testing ForeFire

This document describes how to run the automated tests for the ForeFire wildfire simulator. Tests are located within the `tests/` directory.

## Test Dependencies

Running the test verification scripts requires:

*   **Python 3**
*   **Python Libraries:** `lxml`, `xarray`, `netCDF4` (and their C dependencies like `libnetcdf-dev`).

Install Python libraries via pip:
```bash
pip3 install lxml xarray netCDF4
```

## Running the Core Test (`runff`)

The primary automated test, validated in our CI pipeline, is located in `tests/runff/`. This test verifies core simulation, save/reload functionality, and NetCDF/KML output generation against reference files.

**To run this test manually:**

1.  Ensure ForeFire is compiled (e.g., via `install-forefire.sh`).
2.  Navigate to the test directory: `cd tests/runff`
3.  Execute the test script: `bash ff-run.bash`

**Test Logic:**

The `ff-run.bash` script:
1.  Runs an initial simulation (`real_case.ff`) generating NetCDF output (`ForeFire.0.nc`) and a reload file (`to_reload.ff`).
2.  Runs a second simulation (`reload_case.ff`) using the reload file, which generates KML output (`real_case.kml`).
3.  Uses Python scripts (`compare_kml.py`, `compare_nc.py`) to compare the generated KML and NetCDF files against reference files (`*.ref`) with numerical tolerance, accounting for minor floating-point variations.
4.  Exits with status 0 on success, non-zero on failure.

## Other Tests

The `tests/` directory contains other subdirectories (`mnh_*`, `python`, `runANN`) for potentially testing specific features like coupled simulations or Python bindings. A main `tests/run.bash` script exists but is not currently fully validated in CI. Refer to specific subdirectories for details if needed.

## Contributing

Please see `CONTRIBUTING.md` for guidelines on contributing to ForeFire, including adding new tests. Report any issues via the repository's issue tracker.