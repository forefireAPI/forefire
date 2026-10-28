"""Validate a ForeFire landscape file against a fuel table.

A landscape ``.nc`` and the fuel table used with it have to agree: every fuel
index painted into the raster must exist in the table, or the simulation "will
likely fail or produce incorrect results". Nothing in the engine reports which
index is missing, so this command does it up front.

The logic that decides what is wrong is kept pure (:func:`build_report`) and is
tested without any NetCDF file; :func:`read_landscape` is the thin adapter that
pulls the same information out of a real ``.nc`` and needs ``netCDF4``.

Run it as ``forefire-validate landscape.nc fuels.csv``.
"""

from __future__ import annotations

import argparse
import sys
from collections import namedtuple

# Variable names ForeFire and its documentation accept for each role, in the
# order they are searched. Kept here so the report can name what it looked for.
FUEL_NAMES = ("fuel", "fuel_index", "land_cover")
ELEVATION_NAMES = ("altitude", "elevation", "dem", "hgt")
WIND_U_NAMES = ("windU", "wind_u", "U")
WIND_V_NAMES = ("windV", "wind_v", "V")

#: A single line of the report. ``level`` is "error", "warning" or "info".
Finding = namedtuple("Finding", ("level", "message"))

#: What :func:`read_landscape` extracts and :func:`build_report` consumes.
#: ``fuel_name``/``elevation_name`` are the matched variable name or ``None``;
#: ``raster_indices`` is the set of fuel indices found in the raster;
#: ``fuel_shape``/``elevation_shape`` are the spatial shapes or ``None``;
#: ``has_wind_u``/``has_wind_v`` say whether a wind component is present.
Landscape = namedtuple(
    "Landscape",
    (
        "variables",
        "fuel_name",
        "raster_indices",
        "fuel_shape",
        "elevation_name",
        "elevation_shape",
        "has_wind_u",
        "has_wind_v",
    ),
)


def parse_fuel_indices(text):
    """Return the set of ``Index`` values declared in a fuel table.

    Accepts the ``;``-separated ``fuels.csv`` layout (a ``,`` separator is
    tolerated as a fallback). The header row is the first non-empty line and is
    identified by its first column being ``Index``.
    """
    indices = set()
    header_seen = False
    for raw in text.splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        sep = ";" if ";" in line else ","
        first = line.split(sep, 1)[0].strip()
        if not header_seen:
            header_seen = True
            if first.lower() == "index":
                continue  # skip the header row
            # No header: fall through and treat this line as data.
        try:
            indices.add(int(float(first)))
        except ValueError:
            # A non-numeric first column that is not the header is not an index.
            continue
    return indices


def _find(names, present):
    """Return the first of ``names`` present in ``present``, else ``None``."""
    for name in names:
        if name in present:
            return name
    return None


def build_report(landscape, table_indices):
    """Compare a :class:`Landscape` against the fuel indices ``table_indices``.

    Returns ``(findings, ok)`` where ``findings`` is a list of :class:`Finding`
    and ``ok`` is ``False`` when any finding is an error. This function does no
    I/O, so it can be tested with hand-built inputs.
    """
    findings = []

    if landscape.fuel_name is None:
        findings.append(
            Finding(
                "error",
                "no fuel index variable found (looked for %s)"
                % ", ".join(FUEL_NAMES),
            )
        )
    else:
        findings.append(
            Finding("info", "fuel variable: %r" % landscape.fuel_name)
        )
        missing = sorted(landscape.raster_indices - table_indices)
        if missing:
            findings.append(
                Finding(
                    "error",
                    "fuel indices in the raster but absent from the table: %s"
                    % ", ".join(str(i) for i in missing),
                )
            )
        else:
            findings.append(
                Finding(
                    "info",
                    "all %d fuel indices are defined in the table"
                    % len(landscape.raster_indices),
                )
            )

    if landscape.elevation_name is None:
        findings.append(
            Finding(
                "warning",
                "no elevation variable (looked for %s); slope will not be "
                "computed" % ", ".join(ELEVATION_NAMES),
            )
        )
    elif (
        landscape.fuel_shape is not None
        and landscape.elevation_shape is not None
        and landscape.fuel_shape != landscape.elevation_shape
    ):
        findings.append(
            Finding(
                "error",
                "fuel %s and elevation %s have different shapes"
                % (landscape.fuel_shape, landscape.elevation_shape),
            )
        )

    if not landscape.has_wind_u or not landscape.has_wind_v:
        findings.append(
            Finding(
                "warning",
                "no wind field in the file; supply wind via parameters or the "
                "trigger command",
            )
        )

    ok = not any(f.level == "error" for f in findings)
    return findings, ok


def _unique_indices(array):
    """Return the set of integer fuel indices in a raster array."""
    import numpy as np

    values = np.ma.compressed(array) if np.ma.isMaskedArray(array) else np.asarray(array)
    return {int(v) for v in np.unique(values)}


def read_landscape(nc_path):
    """Read the fields :func:`build_report` needs from a NetCDF landscape file.

    Requires ``netCDF4``. Raises :class:`SystemExit` with an actionable message
    if it is not installed, since it is an optional dependency of the wheel.
    """
    try:
        import netCDF4
    except ImportError:  # pragma: no cover - depends on the environment
        raise SystemExit(
            "reading a landscape file needs the netCDF4 package: "
            "pip install netCDF4"
        )

    with netCDF4.Dataset(nc_path) as ds:
        variables = set(ds.variables)
        fuel_name = _find(FUEL_NAMES, variables)
        elevation_name = _find(ELEVATION_NAMES, variables)

        raster_indices = set()
        fuel_shape = None
        if fuel_name is not None:
            fuel_var = ds.variables[fuel_name]
            raster_indices = _unique_indices(fuel_var[:])
            fuel_shape = tuple(int(n) for n in fuel_var.shape if n > 1)

        elevation_shape = None
        if elevation_name is not None:
            elevation_shape = tuple(
                int(n) for n in ds.variables[elevation_name].shape if n > 1
            )

    return Landscape(
        variables=variables,
        fuel_name=fuel_name,
        raster_indices=raster_indices,
        fuel_shape=fuel_shape,
        elevation_name=elevation_name,
        elevation_shape=elevation_shape,
        has_wind_u=_find(WIND_U_NAMES, variables) is not None,
        has_wind_v=_find(WIND_V_NAMES, variables) is not None,
    )


def format_report(findings):
    """Render findings as aligned ``LEVEL: message`` lines."""
    marks = {"error": "ERROR", "warning": "WARN ", "info": "ok   "}
    return "\n".join("%s  %s" % (marks[f.level], f.message) for f in findings)


def validate_files(nc_path, fuels_path):
    """Validate ``nc_path`` against ``fuels_path``; return ``(findings, ok)``."""
    with open(fuels_path, "r") as handle:
        table_indices = parse_fuel_indices(handle.read())
    landscape = read_landscape(nc_path)
    return build_report(landscape, table_indices)


def main(argv=None):
    """Console entry point for ``forefire-validate``."""
    parser = argparse.ArgumentParser(
        prog="forefire-validate",
        description="Check a ForeFire landscape .nc against a fuel table.",
    )
    parser.add_argument("landscape", help="path to the landscape NetCDF file")
    parser.add_argument("fuels", help="path to the fuel table (e.g. fuels.csv)")
    args = parser.parse_args(argv)

    findings, ok = validate_files(args.landscape, args.fuels)
    print(format_report(findings))
    if ok:
        print("\nlandscape is consistent with the fuel table")
    else:
        print("\nlandscape has errors that will break the simulation")
    return 0 if ok else 1


if __name__ == "__main__":  # pragma: no cover
    sys.exit(main())
