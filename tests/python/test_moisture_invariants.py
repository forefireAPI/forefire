#!/usr/bin/env python3
"""Invariant tests for dead fuel moisture in ForeFire propagation models.

Unlike ``tests/runff``, this suite holds no reference output. Every assertion
here is a property that follows analytically from the published spread
equations, so it stays valid across recalibration, refactoring, and the
planned switch from a static ``fuel.Md`` column to a dynamic dead-moisture
field. A test that compares against frozen ForeFire output cannot tell a fix
from a regression; these can.

Invariants
----------
finite        ROS is a real number for every moisture in [0, 3]. Never NaN.
monotonic     ROS decreases as dead fuel moisture rises, all else equal.
extinction    ROS reaches 0 at Md >= me, for models that define a moisture
              of extinction, and stays 0 above it.
responsive    A dynamic dead-moisture layer actually reaches the model
              (skipped until dynamic Md exists -- see PLUMBING below).
resolved      DataBroker finds an optimised getter for every property the
              model registers, i.e. no silent un-optimised fallback.

Why ROS is probed through a simulation
--------------------------------------
``PropagationModel::getSpeed`` is not exposed to Python, so each probe runs a
short spread from a point ignition over uniform fuel, flat ground and uniform
wind, then measures how far the front travelled. Front displacement over a
fixed duration is monotone in ROS, and works for every propagation model
without knowing its parameter set.

Why every probe forks
---------------------
The C++ core keeps mutable global state -- FireDomain's model registries and
SimulationParameters, as ``pyproject.toml`` notes when explaining why the
free-threaded build is skipped. A second ``ForeFire()`` in the same process
inherits the first one's parameters, and the whole sweep then returns one
identical displacement regardless of moisture. Running the sweep in-process
produces a test that passes or fails for reasons unrelated to its assertions,
so each probe is executed in a fresh interpreter via ``--probe``.

``minSpeed`` is forced to 0. At its default of 0.005 m/s (see
src/SimulationParameters.cpp:378) FireNode::update refuses to move a node
below the floor (src/FireNode.cpp:205), which would mask genuine extinction
behind the same zero displacement.

NaN detection
-------------
A NaN ROS does *not* show up as a NaN displacement: ``speed > minSpeed`` is
false for NaN, so the node stops and the fire merely looks extinguished. The
finiteness check therefore scans the raw ``print[]`` text, because
``FireNode::toString`` (src/FireNode.cpp:750) emits ``vel=`` from a velocity
that is assigned unconditionally at src/FireNode.cpp:201, before the
``minSpeed`` gate. A NaN speed reaches the printed output.

PLUMBING
--------
``DataBroker`` does not raise on an unknown property name. It prints
"WARNING: could not find an optimized property getter for ..." and falls back
to an un-optimised path (src/DataBroker.cpp:161-170), so a typo in a
registered property name yields stale values rather than an error. The
``resolved`` test asserts that warning never appears.

Usage
-----
    python3 tests/python/test_moisture_invariants.py          # run all
    python3 tests/python/test_moisture_invariants.py -v       # per-probe ROS
"""

import argparse
import ctypes
import io
import json
import math
import os
import re
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from contextlib import contextmanager

# --------------------------------------------------------------------------
# Fuel tables
#
# One burnable row, everything but the moisture column held fixed, so a sweep
# varies exactly one input. Values are taken from the Mediterranean shrub row
# (index 82) of tests/runff/fuels.csv and from the SH fuel group of
# pyforefire.helpers.RothermelAndrews2018FuelTable.
# --------------------------------------------------------------------------

FUEL_INDEX = 82

_BALBI_HEADER = (
    "Index;Rhod;Rhol;Md;Ml;sd;sl;e;Sigmad;Sigmal;stoch;RhoA;Ta;Tau0;"
    "Deltah;DeltaH;Cp;Cpa;Ti;X0;r00;Blai;me"
)

_ANDREWS_HEADER = (
    "Index;fl1h_tac;fd_ft;Dme_pc;SAVcar_ftinv;H_BTUlb;fuelDens_lbft3;"
    "totMineral_r;effectMineral_r;mdOnDry1h_r"
)


def balbi_table(md, me=0.30):
    """Rothermel / Balbi column set. Moisture is a dimensionless ratio."""
    row = (
        f"{FUEL_INDEX};614.0;613.0;{md!r};1.0;4287.0;5738.0;0.4;1.378;0.174;"
        f"8.3;1.0;300;70000;18727000.0;18727000.0;1800;1000;600;0.3;"
        f"2.5e-05;4.0;{me!r}"
    )
    return _BALBI_HEADER + "\n" + row


def andrews_table(md, me=0.30):
    """RothermelAndrews2018 column set. `Dme_pc` is a percentage, not a ratio."""
    row = (
        f"{FUEL_INDEX};1.74;1.0;{me * 100.0!r};1550.0;8000.0;32.0;"
        f"0.0555;0.010;{md!r}"
    )
    return _ANDREWS_HEADER + "\n" + row


class ModelSpec:
    """A propagation model plus what its equations promise about moisture."""

    def __init__(self, name, table, has_extinction):
        self.name = name
        self.table = table
        # True when the model defines a moisture of extinction above which
        # ROS is exactly zero. Balbi has no such threshold: its ROS decays
        # smoothly with Md and never reaches zero analytically.
        self.has_extinction = has_extinction

    def __repr__(self):
        return f"<{self.name}>"


MODELS = (
    ModelSpec("Rothermel", balbi_table, has_extinction=True),
    ModelSpec("RothermelAndrews2018", andrews_table, has_extinction=True),
    ModelSpec("BalbiNov2011", balbi_table, has_extinction=False),
    # These three register "moisture" alongside their fuel properties. Until
    # the "moist" prefix in registerPropagationModel was narrowed to "moist.",
    # that single registration was served by the five-slot
    # getMoisturesProperties, shifting every later property by four and
    # leaving each model unable to see its own fuel table at all.
    ModelSpec("Balbi2020", balbi_table, has_extinction=False),
    ModelSpec("Balbi2015", balbi_table, has_extinction=False),
    ModelSpec("BalbiNov2011TMdMl", balbi_table, has_extinction=False),
)

ME = 0.30  # moisture of extinction used throughout
LIVE_MOISTURE = 1.0  # matches the Ml column of the tables above

# --------------------------------------------------------------------------
# Probe
# --------------------------------------------------------------------------

DOMAIN = 2000.0  # m, square
GRID = 100  # cells per side
WIND = 2.0  # m/s, along +x
IGNITION = (DOMAIN / 2.0, DOMAIN / 2.0)

# Run duration is calibrated per model rather than fixed. Spread rates
# differ by more than an order of magnitude between models, and a single
# duration either drives the fast ones off the domain edge -- clipping the
# dry end of the sweep -- or leaves the slow ones barely clear of the
# initial front, where perimeterResolution quantises the displacement and
# adjacent moistures collide. Calibrating keeps every model in the range
# where displacement actually resolves ROS, and survives recalibration of
# any model without anyone editing this file.
CALIB_DURATION = 60.0  # s, the trial run used to estimate ROS
TARGET_SPREAD = 600.0  # m, comfortably inside the 1000 m half-width
MIN_SPREAD = 300.0  # m, below this adjacent moistures start to collide
MAX_SPREAD = 800.0  # m, above this the front is closing on the domain edge
MIN_DURATION = 5.0  # s, floor for a very fast model
MAX_DURATION = 3600.0  # s, ceiling for a very slow model
CALIB_ROUNDS = 4  # Balbi fronts accelerate, so one round can undershoot

# Moisture far above any model's extinction threshold. Its displacement is
# the "did not spread" floor: a point ignition always lays down an initial
# front of finite size before the first update, so extinction shows up as
# displacement equal to this floor, not as exactly zero.
FLOOR_MD = 10.0
FLOOR_TOL = 1.05  # 5% slack over the floor still counts as extinguished

_SENTINEL = "@@PROBE@@"

_LOC_RE = re.compile(r"loc=\(\s*([-+0-9.eEnaif]+)\s*,\s*([-+0-9.eEnaif]+)")
_NONFINITE_RE = re.compile(r"-?\b(nan|inf|Inf|NaN|IND)\b", re.IGNORECASE)


@contextmanager
def captured_native_output():
    """Capture stdout written by the C++ core, not just by Python.

    ForeFire's warnings go to std::cout inside the extension module, which
    never passes through sys.stdout. Redirecting requires the file
    descriptor, so dup2 onto a temp file and restore afterwards.
    """
    libc = ctypes.CDLL(None)
    saved = os.dup(1)
    with tempfile.TemporaryFile(mode="w+b") as tmp:
        try:
            sys.stdout.flush()  # Python's own buffer; fflush only covers libc
            libc.fflush(None)
            os.dup2(tmp.fileno(), 1)
            buf = io.StringIO()
            yield buf
        finally:
            sys.stdout.flush()
            libc.fflush(None)
            os.dup2(saved, 1)
            os.close(saved)
            tmp.seek(0)
            buf.write(tmp.read().decode("utf-8", "replace"))


class Probe:
    """Result of one spread run."""

    def __init__(self, distance, raw, native, duration):
        self.distance = distance  # m travelled by the furthest front node
        self.raw = raw  # concatenated print[] output
        self.native = native  # stdout emitted by the C++ core
        self.duration = duration  # s of simulated time

    @property
    def ros(self):
        """Mean ROS along the fastest ray, m/s."""
        return self.distance / self.duration

    @property
    def has_nonfinite(self):
        return bool(_NONFINITE_RE.search(self.raw))


_DURATIONS = {}


def duration_for(model):
    """Pick a run duration that puts this model's driest case near TARGET_SPREAD.

    Iterated rather than solved in one step: a Balbi front accelerates as it
    develops, so ROS measured over a short trial underestimates the eventual
    rate and the first duration overshoots into the domain edge.
    """
    if model.name in _DURATIONS:
        return _DURATIONS[model.name]

    floor = probe(model, FLOOR_MD, duration=CALIB_DURATION).distance
    duration = CALIB_DURATION
    for _ in range(CALIB_ROUNDS):
        reached = probe(model, DRY_SWEEP[0], duration=duration).distance
        if MIN_SPREAD <= reached <= MAX_SPREAD:
            break
        ros = max((reached - floor) / duration, 1e-6)
        # Clamped to MIN_DURATION, not CALIB_DURATION: a model fast enough to
        # saturate the domain inside the trial needs a *shorter* run than the
        # trial, and clamping at the trial length would leave it clipped.
        duration = min(max(TARGET_SPREAD / ros, MIN_DURATION), MAX_DURATION)

    _DURATIONS[model.name] = duration
    return duration


def probe(model, md, *, me=ME, wind=WIND, duration=None, dead_moisture_layer=None):
    """Spread a fire and report how far the furthest front node got.

    Runs in a forked interpreter; see "Why every probe forks" above.

    `dead_moisture_layer`, when given, is a constant added as a `deadMoisture`
    scalar layer. It is ignored by the current code -- that is exactly what
    the `responsive` test detects.
    """
    if duration is None:
        duration = duration_for(model)
    spec = {
        "model": model.name,
        "md": md,
        "me": me,
        "wind": wind,
        "duration": duration,
        "dead_moisture_layer": dead_moisture_layer,
    }
    proc = subprocess.run(
        [sys.executable, os.path.abspath(__file__), "--probe", json.dumps(spec)],
        capture_output=True, text=True, timeout=300,
    )
    for line in proc.stdout.splitlines():
        if line.startswith(_SENTINEL):
            payload = json.loads(line[len(_SENTINEL):])
            return Probe(payload["distance"], payload["raw"], payload["native"],
                         duration)
    raise RuntimeError(
        f"probe subprocess produced no result (exit {proc.returncode})\n"
        f"stdout: {proc.stdout[-2000:]}\nstderr: {proc.stderr[-2000:]}"
    )


def probe_many(model, mds, **kw):
    """Run a moisture sweep, one subprocess per point, in parallel."""
    with ThreadPoolExecutor(max_workers=min(8, (os.cpu_count() or 2))) as pool:
        return list(pool.map(lambda md: probe(model, md, **kw), mds))


def _probe_in_process(spec):
    """The actual simulation. Only ever called in a freshly forked child."""
    import numpy as np
    import pyforefire as forefire

    model = next(m for m in MODELS if m.name == spec["model"])
    md, me = spec["md"], spec["me"]
    wind, duration = spec["wind"], spec["duration"]
    dead_moisture_layer = spec["dead_moisture_layer"]

    ff = forefire.ForeFire()

    ff["fuelsTable"] = model.table(md, me)
    ff["propagationModel"] = model.name

    # No lower clamp: extinction has to be observable as zero displacement.
    ff["minSpeed"] = 0.0
    ff["windReductionFactor"] = 1.0
    ff["propagationSpeedAdjustmentFactor"] = 1.0

    # Front-tracking numerics. Fixed across the sweep so that any change in
    # displacement is attributable to moisture alone.
    ff["spatialIncrement"] = 1.0
    ff["perimeterResolution"] = 15.0
    ff["minimalPropagativeFrontDepth"] = 20.0
    ff["initialFrontDepth"] = 5.0
    # relax=1 makes FireNode::update take the model's speed directly
    # (src/FireNode.cpp:195). Any relaxation below 1 blends in the ignition
    # velocity, which decays geometrically but never reaches zero, so a fully
    # extinguished front still creeps one spatialIncrement per step and
    # extinction becomes unobservable.
    ff["relax"] = 1.0
    ff["smoothing"] = 0
    ff["bmapLayer"] = 1
    ff["defaultHeatType"] = 0
    ff["nominalHeatFlux"] = 100000
    ff["burningDuration"] = 100

    ff["SWx"] = 0.0
    ff["SWy"] = 0.0
    ff["Lx"] = DOMAIN
    ff["Ly"] = DOMAIN
    ff["atmoNX"] = GRID
    ff["atmoNY"] = GRID

    fuel_map = np.full((1, 1, GRID, GRID), FUEL_INDEX, dtype=np.int32)
    zeros = np.zeros((1, 2, GRID, GRID))
    ones = np.zeros((1, 2, GRID, GRID))
    ones[0, 0, :, :] = 1.0

    with captured_native_output() as native:
        ff.execute(
            f"FireDomain[sw=(0.,0.,0.);ne=({DOMAIN},{DOMAIN},0);t=0]"
        )
        ff.addLayer("propagation", model.name, "propagationModel")
        ff.addIndexLayer(
            "table", "fuel", 0.0, 0.0, 0, DOMAIN, DOMAIN, 0, fuel_map
        )
        ff.addScalarLayer(
            "windScalDir", "windU", 0.0, 0.0, 0, DOMAIN, DOMAIN, 0, ones
        )
        ff.addScalarLayer(
            "windScalDir", "windV", 0.0, 0.0, 0, DOMAIN, DOMAIN, 0, zeros
        )
        # Models such as Balbi2020 register "temperature" and "moisture"
        # (the latter meaning live fuel moisture) beside their fuel
        # properties. Supplying both unconditionally keeps every model on the
        # same footing; models that do not register them never read them.
        ff.addScalarLayer(
            "data", "temperature", 0.0, 0.0, 0, DOMAIN, DOMAIN, 0,
            np.full((1, 1, GRID, GRID), 300.0),
        )
        ff.addScalarLayer(
            "data", "moisture", 0.0, 0.0, 0, DOMAIN, DOMAIN, 0,
            np.full((1, 1, GRID, GRID), LIVE_MOISTURE),
        )
        if dead_moisture_layer is not None:
            layer = np.full((1, 1, GRID, GRID), float(dead_moisture_layer))
            ff.addScalarLayer(
                "data", "deadMoisture", 0.0, 0.0, 0, DOMAIN, DOMAIN, 0, layer
            )

        ff.execute(f"trigger[wind;loc=(0.,0.,0.);vel=({wind},0.,0.);t=0]")
        ff.execute(
            f"startFire[loc=({IGNITION[0]},{IGNITION[1]},0.);t=0.]"
        )

        raw = ff.execute("print[]")
        ff.execute(f"goTo[t={duration}]")
        raw += ff.execute("print[]")

    return {
        "distance": _max_displacement(raw),
        "raw": raw,
        "native": native.getvalue(),
    }


def _max_displacement(raw):
    """Furthest distance any front node reached from the ignition point."""
    best = 0.0
    for xs, ys in _LOC_RE.findall(raw):
        try:
            x, y = float(xs), float(ys)
        except ValueError:
            continue  # a non-finite coordinate; has_nonfinite reports it
        if not (math.isfinite(x) and math.isfinite(y)):
            continue
        best = max(best, math.hypot(x - IGNITION[0], y - IGNITION[1]))
    return best


# --------------------------------------------------------------------------
# Invariants
# --------------------------------------------------------------------------

# Sampled well below the moisture of extinction, where every model is smooth
# and strictly decreasing.
DRY_SWEEP = (0.02, 0.06, 0.10, 0.14, 0.18, 0.22, 0.28)

# At and above the moisture of extinction. Includes values a dynamic
# dead-moisture field produces routinely after rain -- 1.0 is 100% moisture
# on a dry-weight basis, ordinary for live-adjacent litter, and 3.0 is the
# kind of value a saturated-fuel parameterisation can emit.
WET_SWEEP = (0.30, 0.32, 0.50, 1.00, 3.00)


def test_finite(model, report):
    """ROS is a real number for every moisture, on both sides of `me`."""
    failures = []
    sweep = DRY_SWEEP + WET_SWEEP
    for md, p in zip(sweep, probe_many(model, sweep)):
        report(f"    Md={md:<5} d={p.distance:8.2f} m  ROS={p.ros:.4f} m/s")
        if p.has_nonfinite:
            failures.append(
                f"Md={md}: non-finite value in front output "
                f"(NaN/inf ROS reaches FireNode::velocity)"
            )
        if not math.isfinite(p.distance):
            failures.append(f"Md={md}: non-finite front displacement")
    return failures


def test_monotonic(model, report):
    """ROS decreases as dead fuel moisture rises, all else equal.

    Holds analytically for Rothermel: the damping polynomial
    1 - 2.59x + 5.11x^2 - 3.52x^3 has derivative -2.59 + 10.22x - 10.56x^2,
    whose discriminant is -4.95, so it is negative everywhere; and the heat
    of preignition Qig = 250 + 1116*Md sits in the denominator. Both terms
    push the same way. Balbi's 1/(1 + a*Md) factor does likewise.
    """
    failures = []
    sweep = DRY_SWEEP + WET_SWEEP
    results = probe_many(model, sweep)

    prev_md, prev = None, None
    for md, p in zip(sweep, results):
        report(f"    Md={md:<5} d={p.distance:8.2f} m  ROS={p.ros:.4f} m/s")
        if prev is not None:
            # Below the moisture of extinction every model is smooth and
            # strictly decreasing. At and above it, models with a threshold
            # plateau at zero, so only require non-increasing there.
            strict = md <= DRY_SWEEP[-1]
            violated = (
                p.distance >= prev.distance if strict
                else p.distance > prev.distance
            )
            if violated:
                failures.append(
                    f"ROS {'did not decrease' if strict else 'increased'} "
                    f"from Md={prev_md} to Md={md}: "
                    f"{prev.distance:.2f} m -> {p.distance:.2f} m"
                )
        prev_md, prev = md, p
    return failures


def test_extinction(model, report):
    """At and above the moisture of extinction the fire must not spread."""
    if not model.has_extinction:
        report("    skipped: model defines no moisture of extinction")
        return []

    failures = []
    floor = probe(model, FLOOR_MD).distance
    limit = floor * FLOOR_TOL
    report(f"    Md={FLOOR_MD:<5} d={floor:8.2f} m  (floor: initial front only)")

    control_md = ME * 0.9
    control = probe(model, control_md)
    report(f"    Md={control_md:<5} d={control.distance:8.2f} m  (control, must burn)")
    if control.distance <= limit:
        failures.append(
            f"control at Md={control_md} did not outrun the floor "
            f"({control.distance:.2f} m vs {floor:.2f} m); the assertions "
            f"below would pass vacuously"
        )

    for md, p in zip(WET_SWEEP, probe_many(model, WET_SWEEP)):
        report(f"    Md={md:<5} d={p.distance:8.2f} m  (must not spread)")
        if p.distance > limit:
            failures.append(
                f"spread {p.distance:.2f} m at Md={md} >= me={ME} "
                f"(floor is {floor:.2f} m)"
            )
    return failures


def test_responsive(model, report):
    """A dynamic dead-moisture layer must actually reach the model.

    Skips until dynamic dead fuel moisture exists. Once `deadMoisture` is a
    registered property, a layer far above the moisture of extinction must
    stop a fire whose fuel-table Md says it should burn.
    """
    dry = probe(model, 0.05)
    forced = probe(model, 0.05, dead_moisture_layer=1.0)
    report(
        f"    table Md=0.05        d={dry.distance:8.2f} m\n"
        f"    + deadMoisture=1.0   d={forced.distance:8.2f} m"
    )
    if abs(forced.distance - dry.distance) < 1e-9:
        report("    skipped: deadMoisture layer is ignored (not implemented yet)")
        return []
    if not model.has_extinction:
        return []
    # Same floor as test_extinction: a point ignition always lays down an
    # initial front, so "stopped" means "no further than the floor".
    floor = probe(model, FLOOR_MD).distance
    if forced.distance > floor * FLOOR_TOL:
        return [
            f"deadMoisture layer at 1.0 (>> me={ME}) did not stop the fire: "
            f"spread {forced.distance:.2f} m (floor is {floor:.2f} m)"
        ]
    return []


def test_resolved(model, report):
    """Every registered property resolves to an optimised getter.

    src/DataBroker.cpp:161-170 warns and silently degrades instead of
    failing, so a mistyped property name would otherwise go unnoticed.
    """
    p = probe(model, 0.10)
    failures = []
    for marker in (
        "could not find an optimized property getter",
        "switched to an un-optimized mode",
    ):
        if marker in p.native:
            failures.append(f"DataBroker fell back: {marker!r}")
    report("    no DataBroker fallback warnings" if not failures else "")
    return failures


TESTS = (
    ("finite", test_finite),
    ("monotonic", test_monotonic),
    ("extinction", test_extinction),
    ("responsive", test_responsive),
    ("resolved", test_resolved),
)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="print every probe's displacement and ROS")
    ap.add_argument("--model", action="append", metavar="NAME",
                    help="restrict to these propagation models")
    ap.add_argument("--test", action="append", metavar="NAME",
                    help="restrict to these invariants")
    ap.add_argument("--probe", metavar="JSON",
                    help=argparse.SUPPRESS)  # internal: run one probe and exit
    args = ap.parse_args(argv)

    if args.probe:
        result = _probe_in_process(json.loads(args.probe))
        print(_SENTINEL + json.dumps(result))
        return 0

    models = MODELS
    if args.model:
        models = tuple(m for m in MODELS if m.name in args.model)
        if not models:
            ap.error(f"no such model; known: {[m.name for m in MODELS]}")
    tests = TESTS
    if args.test:
        tests = tuple(t for t in TESTS if t[0] in args.test)
        if not tests:
            ap.error(f"no such test; known: {[t[0] for t in TESTS]}")

    def report(msg):
        if args.verbose and msg:
            print(msg)

    total = 0
    for model in models:
        print(f"\n=== {model.name} ===")
        for name, fn in tests:
            print(f"  {name}")
            try:
                failures = fn(model, report)
            except Exception as exc:  # a crash is a failure, not an error
                failures = [f"raised {type(exc).__name__}: {exc}"]
            if failures:
                total += len(failures)
                for f in failures:
                    print(f"    FAIL: {f}")
            else:
                print("    ok")

    print()
    if total:
        print(f"FAILED: {total} invariant violation(s)")
        return 1
    print("All invariants hold.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
