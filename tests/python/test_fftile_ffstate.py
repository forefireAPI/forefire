"""FFTile landscapes and FFState checkpoints driven from `pyforefire`.

Writes one small synthetic tile, so it needs no download. Each simulation runs
in its own `ForeFireRunner` worker: the kernel keeps one domain per process,
and a checkpoint restore needs an empty session.

    python tests/python/test_fftile_ffstate.py

Exit status is 0 only if every check passes.
"""

import sys
import tempfile
import traceback
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "tiles"))
sys.path.insert(0, str(ROOT / "tests" / "ffstate"))
from fftile import write_tile  # noqa: E402
from smoke import chunks  # noqa: E402

from pyforefire import ForeFireRunner  # noqa: E402

FUEL = 2
ALTITUDE = 120.0
# Chunks that are the fire state; output settings and bookkeeping are not.
STATE_CHUNKS = ("domain", "objects.ff", "events", "arrival")


def write_landscape(directory):
    flat = np.zeros((16, 16), np.float32)
    fuel = np.full((64, 64), FUEL, np.uint8)
    write_tile(directory / "42_8.5.fftile", 42, 8.5, fuel, flat + ALTITUDE,
               flat, flat, flat + 1, flat, flat, flat + 1, timestamp=0)


def session(case, subdivisions):
    """A worker with the case parameters set and no domain yet."""
    ff = ForeFireRunner()
    # An empty value: setParameter must not read past its argument list.
    ff.execute(f"setParameters[caseDirectory={case};ForeFireDataDirectory=]")
    ff.execute("setParameters[propagationModel=Rothermel;"
               "fuelsTable=STDlandIndexedFuelsTable;perimeterResolution=24;"
               "spatialIncrement=6;minSpeed=0;outputsUpdate=0;bmapLayer=1;"
               f"arrivalMapSubdivisions={subdivisions}]")
    return ff


def ignite(ff, tiles):
    ff.execute(f"loadData[provider={tiles};center=(8.8,42.15);size=(.08,.08);"
               "date=2025-06-01T12:00:00Z]")
    ff.execute("trigger[wind;vel=(6,2,0)]")
    ff.execute("startFire[lonlat=(8.8,42.15,0);t=0]")


def same_state(a, b):
    left, right = chunks(a), chunks(b)
    return [name for name in STATE_CHUNKS if left[name] != right[name]]


def check_tile_layers(case, tiles):
    ff = session(case, 0)
    try:
        ignite(ff, tiles)
        fuel = ff.getDoubleArray("fuel")
        altitude = ff.getDoubleArray("altitude")
        assert fuel.size and np.all(fuel == FUEL), "fuel layer does not hold the tile's fuel"
        assert altitude.size and np.allclose(altitude, ALTITUDE), "altitude layer does not hold the tile's"
        ff.execute("step[dt=300]")
        arrival = ff.getDoubleArray("BMap")
        assert np.isfinite(arrival).sum() > 0, "no arrival time after 300 s of spread"
        assert ff.execute("print[]").count("FireNode") > 0
    finally:
        ff.close()


def check_restore(case, tiles, subdivisions):
    """A checkpoint restored in a fresh worker continues like the original."""
    case = case / f"subdivisions-{subdivisions}"
    case.mkdir()
    seed, continuous, resumed = (case / name for name in
                                 ("seed.ffstate", "continuous.ffstate", "resumed.ffstate"))
    original = session(case, subdivisions)
    restored = session(case, subdivisions)
    try:
        ignite(original, tiles)
        original.execute("step[dt=120.25]")
        original.execute(f"saveState[file={seed}]")
        original.execute("step[dt=300]")
        original.execute(f"saveState[file={continuous}]")

        out = restored.execute(f"loadState[file={seed}]")
        assert "Loaded FFState" in out, out
        restored.execute("step[dt=300]")
        restored.execute(f"saveState[file={resumed}]")

        differ = same_state(continuous, resumed)
        assert not differ, f"restored run differs from the original in {differ}"
        assert np.array_equal(original.getDoubleArray("BMap"), restored.getDoubleArray("BMap"),
                              equal_nan=True), "arrival times read from Python differ"
        assert original.execute("print[]") == restored.execute("print[]")

        # A loaded session is never replaced, and stays usable.
        out = restored.execute(f"loadState[file={seed}]")
        assert "domain exists" in out, out
        restored.execute("step[dt=60]")
    finally:
        original.close()
        restored.close()


def check_corrupt_state(case, tiles):
    ff = session(case, 0)
    try:
        ignite(ff, tiles)
        ff.execute("step[dt=60]")
        good = case / "good.ffstate"
        ff.execute(f"saveState[file={good}]")
    finally:
        ff.close()
    bad = bytearray(good.read_bytes())
    bad[-1] ^= 1
    (case / "bad.ffstate").write_bytes(bad)
    ff = session(case, 0)
    try:
        out = ff.execute(f"loadState[file={case / 'bad.ffstate'}]")
        assert "checksum mismatch" in out, out
        # Nothing was published: the good checkpoint still loads.
        out = ff.execute(f"loadState[file={good}]")
        assert "Loaded FFState" in out, out
    finally:
        ff.close()


def main():
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        tiles = tmp / "tiles"
        tiles.mkdir()
        write_landscape(tiles)
        checks = [
            ("tile layers read from Python", lambda: check_tile_layers(tmp, tiles)),
            ("restore, Cartesian arrival map", lambda: check_restore(tmp, tiles, 0)),
            ("restore, geographic arrival map", lambda: check_restore(tmp, tiles, 256)),
            ("corrupt checkpoint refused", lambda: check_corrupt_state(tmp, tiles)),
        ]
        failed = 0
        for name, check in checks:
            try:
                check()
                print(f"PASS  {name}")
            except Exception:
                failed += 1
                print(f"FAIL  {name}")
                traceback.print_exc()
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
