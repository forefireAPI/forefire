"""Smoke test for tools/preprocessing/genForeFireCase.py.

The landscape file is the thing standing between a new user and their first
simulation, so this checks the whole path rather than the writer alone: build
a landscape, hand it to ForeFire, ignite it, and step.

It also pins the axis order. The 3-D and 4-D paths used to declare their
dimensions in one order and assign in another: a 4-D field only broadcast when
NY == NT and NX == NZ, and a 3-D field never got its NT dimension created.

    python3 tests/python/test_genforefirecase.py

Needs numpy, netCDF4 and a built pyforefire. Skips rather than fails when
pyforefire is missing, so it stays runnable on a machine with no build.
"""

import os
import sys
import tempfile
import traceback

sys.path.insert(0, os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "..",
    "tools", "preprocessing"))

import numpy as np  # noqa: E402
from netCDF4 import Dataset  # noqa: E402

from genForeFireCase import FiretoNC, REQUIRED_PARAMETERS  # noqa: E402

NX = 100
NY = 100


def domain_properties(nx=NX, ny=NY, resolution=10.0):
    return {'SWx': 0., 'SWy': 0., 'SWz': 0.,
            'Lx': nx * resolution, 'Ly': ny * resolution, 'Lz': 0.,
            't0': 0., 'Lt': np.inf}


def parameters_properties():
    return {'date': "2026-08-12T12:00:00Z", 'duration': 3600,
            'refYear': 2026, 'refDay': 224,
            'year': 2026, 'month': 8, 'day': 12}


def test_writes_a_loadable_landscape():
    """The generated file loads in ForeFire, ignites and spreads."""
    try:
        import pyforefire
    except ImportError:
        return ["SKIP: pyforefire not importable"]

    failures = []
    with tempfile.TemporaryDirectory() as workdir:
        path = os.path.join(workdir, "landscape.nc")

        fuel = np.full((NY, NX), 1, dtype=np.int32)
        elevation = np.zeros((NY, NX))
        wind = {"zonal": np.full((NY, NX), 2.0),
                "meridian": np.zeros((NY, NX))}

        FiretoNC(path, domain_properties(), parameters_properties(),
                 fuel, elevation=elevation, wind=wind)

        with Dataset(path) as ds:
            for name in ('fuel', 'altitude', 'windU', 'windV',
                         'domain', 'parameters'):
                if name not in ds.variables:
                    failures.append("landscape is missing variable %s" % name)
        if failures:
            return failures

        ff = pyforefire.ForeFire()
        ff.setString("fuelsTableFile",
                     os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                  "..", "runff", "fuels.csv"))
        ff.setString("NetCDFfile", path)
        ff.execute("FireDomain[sw=(0,0,0);ne=(1000,1000,0);t=0]")
        ff.addLayer("propagation", "Rothermel", "propagationModel")
        ff.execute("startFire[loc=(500,500,0);t=0]")
        for _ in range(5):
            ff.execute("step[dt=100]")

        nodes = ff.execute("print[]").count("FireNode")
        if nodes == 0:
            failures.append("no fire nodes after stepping a loaded landscape")

    return failures


def test_four_dimensional_field():
    """A 4-D field keeps its shape and its values.

    Deliberately uses four different lengths: with NY == NT and NX == NZ the
    old code's broadcast succeeded by accident.
    """
    failures = []
    nt, nz, ny, nx = 5, 2, 3, 4

    with tempfile.TemporaryDirectory() as workdir:
        path = os.path.join(workdir, "landscape.nc")

        fuel = np.full((ny, nx), 1, dtype=np.int32)
        # Every element distinct, so a reordering cannot go unnoticed.
        zonal = np.arange(nt * nz * ny * nx, dtype=float).reshape(
            (nt, nz, ny, nx))
        meridian = np.zeros((nt, nz, ny, nx))

        FiretoNC(path, domain_properties(nx, ny), parameters_properties(),
                 fuel, wind={"zonal": zonal, "meridian": meridian})

        with Dataset(path) as ds:
            got = np.array(ds.variables['windU'][:])

        if got.shape != (nt, nz, ny, nx):
            failures.append("windU has shape %s, expected %s"
                            % (got.shape, (nt, nz, ny, nx)))
        elif not np.array_equal(got, zonal):
            failures.append("windU values do not match the input")

    return failures


def test_three_dimensional_field():
    """A 3-D field gains a leading NT of 1 and keeps its values.

    The old code never created the NT dimension on this path, so writing a
    3-D field failed outright.
    """
    failures = []
    nz, ny, nx = 2, 3, 4

    with tempfile.TemporaryDirectory() as workdir:
        path = os.path.join(workdir, "landscape.nc")

        fuel = np.full((ny, nx), 1, dtype=np.int32)
        zonal = np.arange(nz * ny * nx, dtype=float).reshape((nz, ny, nx))

        FiretoNC(path, domain_properties(nx, ny), parameters_properties(),
                 fuel, wind={"zonal": zonal,
                             "meridian": np.zeros((nz, ny, nx))})

        with Dataset(path) as ds:
            got = np.array(ds.variables['windU'][:])

        if got.shape != (1, nz, ny, nx):
            failures.append("windU has shape %s, expected %s"
                            % (got.shape, (1, nz, ny, nx)))
        elif not np.array_equal(got[0], zonal):
            failures.append("3-D windU values do not match the input")

    return failures


def test_missing_parameter_is_reported_before_writing():
    """A missing key names itself, and leaves no half-written file behind."""
    failures = []
    incomplete = parameters_properties()
    del incomplete['refDay']

    with tempfile.TemporaryDirectory() as workdir:
        path = os.path.join(workdir, "landscape.nc")
        try:
            FiretoNC(path, domain_properties(), incomplete,
                     np.full((NY, NX), 1, dtype=np.int32))
            failures.append("a missing parameter key did not raise")
        except KeyError as error:
            if 'refDay' not in str(error):
                failures.append("the error does not name the missing key: %s"
                                % error)
        if os.path.exists(path):
            failures.append("a file was written despite the missing key")

    return failures


def main():
    tests = [test_writes_a_loadable_landscape,
             test_four_dimensional_field,
             test_three_dimensional_field,
             test_missing_parameter_is_reported_before_writing]

    failures = []
    for test in tests:
        try:
            result = test()
        except Exception:
            result = ["%s raised:\n%s" % (test.__name__, traceback.format_exc())]
        for line in result:
            if line.startswith("SKIP:"):
                print("%s: %s" % (test.__name__, line))
            else:
                failures.append("%s: %s" % (test.__name__, line))
        if not result:
            print("%s: ok" % test.__name__)

    if failures:
        print("\nFAILED with %d problem(s):" % len(failures))
        for failure in failures:
            print("  - %s" % failure)
        return 1
    print("\nOK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
