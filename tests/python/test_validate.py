#!/usr/bin/env python3
"""Tests for the ``forefire-validate`` landscape checker.

The decision logic in ``pyforefire._validate`` is pure, so it is tested here
with hand-built inputs and no NetCDF file — and the module is loaded straight
from its source path, so this suite does not need the compiled ``_pyforefire``
extension either. A real ``.nc`` round-trip runs too, but only when ``netCDF4``
is installed; it is skipped otherwise rather than failing.
"""

import importlib.util
import os
import sys
import tempfile

_MODULE_PATH = os.path.join(
    os.path.dirname(os.path.dirname(os.path.dirname(__file__))),
    "bindings", "python", "src", "pyforefire", "_validate.py",
)


def _load_validate():
    spec = importlib.util.spec_from_file_location("_ff_validate", _MODULE_PATH)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


V = _load_validate()


def _landscape(**overrides):
    """A consistent Landscape (fuel + elevation + wind, matching shapes)."""
    fields = dict(
        variables={"fuel", "altitude", "windU", "windV"},
        fuel_name="fuel",
        raster_indices={0, 1, 2},
        fuel_shape=(100, 100),
        elevation_name="altitude",
        elevation_shape=(100, 100),
        has_wind_u=True,
        has_wind_v=True,
    )
    fields.update(overrides)
    return V.Landscape(**fields)


def _errors(findings):
    return [f.message for f in findings if f.level == "error"]


def _levels(findings):
    return {f.level for f in findings}


# --- parse_fuel_indices -----------------------------------------------------

def test_parse_semicolon_table():
    text = "Index;Rhod;Ml\n0;563;1.0\n1;563;1.0\n2;614;1.0\n"
    got = V.parse_fuel_indices(text)
    return [] if got == {0, 1, 2} else [f"parsed {got}, expected {{0,1,2}}"]


def test_parse_skips_header_and_blanks():
    text = "\nIndex;a\n1;x\n\n# comment\n7;y\n"
    got = V.parse_fuel_indices(text)
    return [] if got == {1, 7} else [f"parsed {got}, expected {{1,7}}"]


def test_parse_comma_fallback_and_no_header():
    got = V.parse_fuel_indices("3,foo\n4,bar\n")
    return [] if got == {3, 4} else [f"parsed {got}, expected {{3,4}}"]


# --- build_report -----------------------------------------------------------

def test_consistent_case_is_ok():
    findings, ok = V.build_report(_landscape(), {0, 1, 2, 3})
    if not ok:
        return [f"expected ok, got errors {_errors(findings)}"]
    if "error" in _levels(findings) or "warning" in _levels(findings):
        return [f"expected only info, got {_levels(findings)}"]
    return []


def test_missing_index_is_error():
    findings, ok = V.build_report(_landscape(raster_indices={0, 1, 9}), {0, 1, 2})
    errs = _errors(findings)
    if ok:
        return ["index 9 missing from table but case reported ok"]
    if not any("9" in m for m in errs):
        return [f"error should name index 9, got {errs}"]
    return []


def test_absent_fuel_variable_is_error():
    findings, ok = V.build_report(
        _landscape(fuel_name=None, raster_indices=set(), fuel_shape=None),
        {0, 1},
    )
    if ok or not any("fuel" in m for m in _errors(findings)):
        return [f"absent fuel var should be an error, got {_errors(findings)}"]
    return []


def test_shape_mismatch_is_error():
    findings, ok = V.build_report(
        _landscape(elevation_shape=(50, 50)), {0, 1, 2}
    )
    if ok or not any("shape" in m for m in _errors(findings)):
        return [f"shape mismatch should be an error, got {_errors(findings)}"]
    return []


def test_missing_elevation_is_warning_not_error():
    findings, ok = V.build_report(
        _landscape(
            variables={"fuel", "windU", "windV"},
            elevation_name=None,
            elevation_shape=None,
        ),
        {0, 1, 2},
    )
    if not ok:
        return [f"missing elevation should not be fatal, got {_errors(findings)}"]
    if "warning" not in _levels(findings):
        return ["missing elevation should produce a warning"]
    return []


def test_missing_wind_is_warning_not_error():
    findings, ok = V.build_report(
        _landscape(
            variables={"fuel", "altitude"},
            has_wind_u=False,
            has_wind_v=False,
        ),
        {0, 1, 2},
    )
    if not ok:
        return [f"missing wind should not be fatal, got {_errors(findings)}"]
    if "warning" not in _levels(findings):
        return ["missing wind should produce a warning"]
    return []


# --- read_landscape (only if netCDF4 is available) --------------------------

def test_read_real_netcdf():
    try:
        import netCDF4
        import numpy as np
    except ImportError:
        print("    (skipped: netCDF4 not installed)")
        return []

    with tempfile.TemporaryDirectory() as d:
        path = os.path.join(d, "case.nc")
        with netCDF4.Dataset(path, "w") as ds:
            ds.createDimension("y", 4)
            ds.createDimension("x", 5)
            fuel = ds.createVariable("fuel", "i4", ("y", "x"))
            fuel[:] = np.array([[0, 1, 2, 1, 0]] * 4)
            alt = ds.createVariable("altitude", "f4", ("y", "x"))
            alt[:] = 0.0

        land = V.read_landscape(path)
        problems = []
        if land.fuel_name != "fuel":
            problems.append(f"fuel_name={land.fuel_name!r}")
        if land.raster_indices != {0, 1, 2}:
            problems.append(f"raster_indices={land.raster_indices}")
        if land.fuel_shape != (4, 5):
            problems.append(f"fuel_shape={land.fuel_shape}")
        if land.has_wind_u or land.has_wind_v:
            problems.append("wind reported present in a file with none")

        _, ok = V.build_report(land, {0, 1, 2})
        if not ok:
            problems.append("consistent real file reported as broken")
        return problems


def main():
    tests = [
        test_parse_semicolon_table,
        test_parse_skips_header_and_blanks,
        test_parse_comma_fallback_and_no_header,
        test_consistent_case_is_ok,
        test_missing_index_is_error,
        test_absent_fuel_variable_is_error,
        test_shape_mismatch_is_error,
        test_missing_elevation_is_warning_not_error,
        test_missing_wind_is_warning_not_error,
        test_read_real_netcdf,
    ]
    total = 0
    for fn in tests:
        print(f"  {fn.__name__}")
        try:
            failures = fn()
        except Exception as exc:
            failures = [f"raised {type(exc).__name__}: {exc}"]
        if failures:
            total += len(failures)
            for f in failures:
                print(f"    FAIL: {f}")
        else:
            print("    ok")

    print()
    if total:
        print(f"FAILED: {total} failure(s)")
        return 1
    print("All validate tests pass.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
