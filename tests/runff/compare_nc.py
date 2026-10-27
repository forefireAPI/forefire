# tests/runff/compare_nc.py
import sys
import numpy as np
import xarray as xr

# Arrival times are compared cell by cell, but a few cells along the front may
# burn on one CPU and not on another, or burn a little earlier, as the front
# itself differs slightly between machines (see compare_kml.py).
# --- Configuration ---
VARIABLE = "arrival_time_of_front"
UNBURNT = -9999
MAX_TIME_DIFF_S = 30.0         # a burnt cell differs if its arrival time moves more
MAX_DIFFERING_FRACTION = 0.01  # share of burnt cells allowed to differ
# ---

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python compare_nc.py <generated_nc_file> <reference_nc_file>", file=sys.stderr)
        sys.exit(2)
    file1_path, file2_path = sys.argv[1], sys.argv[2]
    try:
        with xr.open_dataset(file1_path, cache=False) as ds1, xr.open_dataset(file2_path, cache=False) as ds2:
            t1, t2 = ds1[VARIABLE].values, ds2[VARIABLE].values
    except Exception as e:
        print(f"Error reading NetCDF: {e}", file=sys.stderr)
        sys.exit(1)
    if t1.shape != t2.shape:
        print(f"Error: {VARIABLE} has shape {t1.shape} vs {t2.shape}", file=sys.stderr)
        sys.exit(1)

    burnt1, burnt2 = t1 != UNBURNT, t2 != UNBURNT
    both = burnt1 & burnt2
    only_one = int((burnt1 ^ burnt2).sum())
    time_diff = np.abs(t1[both] - t2[both])
    late = int((time_diff > MAX_TIME_DIFF_S).sum())
    burnt = int((burnt1 | burnt2).sum())
    fraction = (only_one + late) / burnt if burnt else 0.0

    print(f"Burnt cells: {int(burnt1.sum())} vs {int(burnt2.sum())}, {only_one} burnt in only one, "
          f"{late} more than {MAX_TIME_DIFF_S:.0f} s apart (max diff {time_diff.max() if time_diff.size else 0:.1f} s); "
          f"{fraction:.2%} of burnt cells differ (max {MAX_DIFFERING_FRACTION:.0%})")
    if fraction <= MAX_DIFFERING_FRACTION:
        print(f"NetCDF comparison successful: {file1_path} matches {file2_path} within tolerance.")
        sys.exit(0)
    print(f"NetCDF comparison failed: {file1_path} differs from {file2_path}.", file=sys.stderr)
    sys.exit(1)
