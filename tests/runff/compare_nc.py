# tests/runff/compare_nc.py
import sys
import xarray as xr

# --- Configuration ---
# Adjust tolerance as needed. Start with something reasonable.
RELATIVE_TOLERANCE = 1e-5
ABSOLUTE_TOLERANCE = 1e-8
# ---

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python compare_nc.py <generated_nc_file> <reference_nc_file>", file=sys.stderr)
        sys.exit(2) # Exit code for usage error

    file1_path = sys.argv[1]
    file2_path = sys.argv[2]

    ds1 = None # Initialize to None
    ds2 = None # Initialize to None
    try:
        # Open datasets using xarray. Ensure they are closed afterwards.
        ds1 = xr.open_dataset(file1_path, cache=False)
        ds2 = xr.open_dataset(file2_path, cache=False)

        # Compare the datasets numerically with tolerance
        # This compares all data variables by default.
        # Use `equal = ds1.equals(ds2)` for exact bit-wise comparison (usually too strict)
        # Use `identical = ds1.identical(ds2)` for exact comparison including attributes etc. (also often too strict)
        xr.testing.assert_allclose(ds1, ds2, rtol=RELATIVE_TOLERANCE, atol=ABSOLUTE_TOLERANCE)

        print(f"NetCDF comparison successful: {file1_path} matches {file2_path} within tolerance.")
        sys.exit(0) # Success

    except FileNotFoundError as e:
        print(f"Error: NetCDF file not found - {e}", file=sys.stderr)
        sys.exit(1)
    except AssertionError as e:
        # This exception is raised by xr.testing.assert_allclose on failure
        print(f"NetCDF comparison failed: {file1_path} differs from {file2_path}.", file=sys.stderr)
        print(f"Details from xarray: {e}", file=sys.stderr)
        sys.exit(1) # Failure - Datasets are different
    except Exception as e:
        # Catch other potential errors (e.g., invalid NetCDF format)
        print(f"An unexpected error occurred during NetCDF comparison: {e}", file=sys.stderr)
        sys.exit(1) # Failure
    finally:
        # Ensure datasets are closed to release file handles
        if ds1 is not None:
            ds1.close()
        if ds2 is not None:
            ds2.close()