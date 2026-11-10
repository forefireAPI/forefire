#!/usr/bin/env python3
"""Compare ForeFire's Farsite model with the FARSITE reference code.

The reference is ``farsite_reference.cpp`` next to this file: FARSITE's own
spread rate, wind/slope vector and ellipse, copied from mbedward/farsite with
only the plumbing changed. It is compiled with the system C++ compiler when
the test starts.

The ForeFire side runs real simulations. Each case lays a circle of 72 nodes
on uniform fuel, uniform wind blowing north and a plane rising to the north,
and reads back what ``Farsite::getSpeed`` was called with and returned, from
the CSV written by ``FarsiteLoggerCSVPath``. The node facing north is the
head, the one facing south the back, the ones facing east or west the flanks.

Two comparisons, both against the reference, never against ForeFire itself:

model     The reference is fed the wind and slope ForeFire handed to the
          model at the head node, converted as the model does (sine to
          tangent, wind back to horizontal, except with splines). Any gap is
          in FarsitePropagationModel.cpp or in the fuel table and moistures.
pipeline  The reference is fed the case as FARSITE sees it: the midflame wind
          and the tangent of the terrain slope. Any gap beyond the model one
          is in how ForeFire derives the normal wind and slope it hands over.

The test fails when the head rate of spread differs beyond REL_TOL in either
comparison. Flank, back and length-to-breadth are only reported: ForeFire has
no ellipse, so a node not facing the wind spreads at the no-wind rate.

Every case runs in its own interpreter, because a second ``ForeFire()`` in a
process inherits the first one's parameters (see test_moisture_invariants.py).

Usage
-----
    python3 tests/python/test_farsite_reference.py           # summary
    python3 tests/python/test_farsite_reference.py -v        # every case
    python3 tests/python/test_farsite_reference.py --scheme splines
"""

import argparse
import json
import math
import os
import shutil
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
REFERENCE_SRC = os.path.join(HERE, "farsite_reference.cpp")

FUELS = range(1, 14)
WINDS = (0.0, 2.0, 5.0, 10.0)  # m/s at midflame
SLOPES = (0.0, 10.0, 30.0)  # degrees
MOISTURES = {  # fractions of dry weight: ones, tens, hundreds, liveh, livew
    "dry": (0.03, 0.04, 0.05, 0.60, 0.90),
    "moderate": (0.06, 0.07, 0.08, 0.90, 1.20),
    "moist": (0.09, 0.10, 0.11, 1.20, 1.50),
}

MPH = 0.44704  # m/s
# The ForeFire log prints 6 significant digits, so the comparison cannot be
# tighter than about 1e-5; anything a porting bug produces is far larger.
REL_TOL = 1e-4

DOMAIN = 400.0  # m, square
GRID = 80  # cells per side
RADIUS = 40.0  # m, the initial circle
NODES = 72  # node 0 faces north, NODES/4 east, NODES/2 south

_SENTINEL = "@@PROBE@@"


# --------------------------------------------------------------------------
# Reference
# --------------------------------------------------------------------------

def build_reference(workdir):
    exe = os.path.join(workdir, "farsite_reference")
    cxx = os.environ.get("CXX") or shutil.which("c++") or shutil.which("g++")
    if cxx is None:
        raise RuntimeError("no C++ compiler found to build farsite_reference.cpp")
    subprocess.run([cxx, "-O2", "-o", exe, REFERENCE_SRC], check=True)
    return exe


def run_reference(exe, rows):
    """rows: (fuel, moistures, wind m/s, slope tangent). Returns dicts, m/s."""
    lines = []
    for fuel, moist, wind, slope in rows:
        args = (fuel,) + tuple(moist) + (wind / MPH, slope)
        lines.append(" ".join(repr(float(a)) if i else str(a)
                              for i, a in enumerate(args)))
    out = subprocess.run([exe], input="\n".join(lines) + "\n",
                         capture_output=True, text=True, check=True).stdout
    keys = ("rateo", "phiw", "phis", "phiew", "head", "lb", "flank", "back")
    results = []
    for line in out.splitlines():
        r = dict(zip(keys, map(float, line.split())))
        for k in ("rateo", "head", "flank", "back"):
            r[k] /= 60.0  # m/min -> m/s
        results.append(r)
    if len(results) != len(rows):
        raise RuntimeError("reference returned %d rows for %d cases"
                           % (len(results), len(rows)))
    return results


# --------------------------------------------------------------------------
# ForeFire
# --------------------------------------------------------------------------

def probe(case):
    proc = subprocess.run(
        [sys.executable, os.path.abspath(__file__), "--probe", json.dumps(case)],
        capture_output=True, text=True, timeout=300,
    )
    for line in proc.stdout.splitlines():
        if line.startswith(_SENTINEL):
            return json.loads(line[len(_SENTINEL):])
    raise RuntimeError(
        f"probe {case} produced no result (exit {proc.returncode})\n"
        f"stdout: {proc.stdout[-2000:]}\nstderr: {proc.stderr[-2000:]}"
    )


def _probe_in_process(case):
    """One case in a fresh interpreter: every getSpeed call at ignition."""
    import numpy as np
    import pyforefire as forefire

    ones, tens, hundreds, liveh, livew = MOISTURES[case["moisture"]]
    wind, slope_deg = case["wind"], case["slope"]

    with tempfile.TemporaryDirectory() as tmp:
        log = os.path.join(tmp, "farsite.csv")
        ff = forefire.ForeFire()
        ff["propagationModel"] = "Farsite"
        ff["normalScheme"] = case["scheme"]
        ff["fuelsTable"] = "STDfarsiteFuelsTable"
        ff["FarsiteLoggerCSVPath"] = log
        ff["moistures.ones"] = ones
        ff["moistures.tens"] = tens
        ff["moistures.hundreds"] = hundreds
        ff["moistures.liveh"] = liveh
        ff["moistures.livew"] = livew
        ff["minSpeed"] = 0.0
        ff["relax"] = 1.0
        ff["spatialIncrement"] = 1.0
        ff["perimeterResolution"] = 10.0
        ff["minimalPropagativeFrontDepth"] = 10.0
        ff["bmapLayer"] = 1
        ff["SWx"] = 0.0
        ff["SWy"] = 0.0
        ff["Lx"] = DOMAIN
        ff["Ly"] = DOMAIN

        ff.execute(f"FireDomain[sw=(0.,0.,0.);ne=({DOMAIN},{DOMAIN},0);t=0]")
        ff.addLayer("propagation", "Farsite", "propagationModel")
        ff.addIndexLayer("table", "fuel", 0.0, 0.0, 0, DOMAIN, DOMAIN, 0,
                         np.full((1, 1, GRID, GRID), case["fuel"], dtype=np.int32))
        u = np.zeros((1, 2, GRID, GRID))
        v = np.zeros((1, 2, GRID, GRID))
        u[0, 0] = 1.0
        v[0, 1] = 1.0
        ff.addScalarLayer("windScalDir", "windU", 0.0, 0.0, 0, DOMAIN, DOMAIN, 0, u)
        ff.addScalarLayer("windScalDir", "windV", 0.0, 0.0, 0, DOMAIN, DOMAIN, 0, v)
        # A plane rising to the north, sampled at cell centres.
        y = (np.arange(GRID) + 0.5) * DOMAIN / GRID
        altitude = np.zeros((1, 1, GRID, GRID))
        altitude[0, 0] = (y * math.tan(math.radians(slope_deg)))[:, None]
        ff.addScalarLayer("data", "altitude", 0.0, 0.0, 0, DOMAIN, DOMAIN, 0,
                          altitude)
        ff.execute(f"trigger[wind;loc=(0.,0.,0.);vel=(0.,{wind},0.);t=0]")

        ff.execute("FireFront[id=2;domain=0;t=0]")
        for k in range(NODES):
            a = math.pi / 2 - 2 * math.pi * k / NODES  # clockwise from north
            x = DOMAIN / 2 + RADIUS * math.cos(a)
            yy = DOMAIN / 2 + RADIUS * math.sin(a)
            ff.execute(
                f"FireNode[domain=0;id={4 + 2 * k};fdepth=5;kappa=0;"
                f"loc=({x},{yy},0);vel=({0.01 * math.cos(a)},"
                f"{0.01 * math.sin(a)},0);t=0;state=init;frontId=2]")
        ff.execute("goTo[t=0.01]")
        del ff  # log lines end in endl, so the file is already flushed

        with open(log) as f:
            header = f.readline().strip().split(";")
            rows = [dict(zip(header, map(float, line.split(";"))))
                    for line in f if line.strip()]
    keep = ("ROS", "rateo", "phiew", "slope", "normalWind")
    return [{k: r[k] for k in keep} for r in rows]


# --------------------------------------------------------------------------
# Comparison
# --------------------------------------------------------------------------

def pick(rows, which):
    """The logged call at the head, back or flank of the circle."""
    def facing(r):  # +1 north (head), -1 south (back), 0 east/west (flank)
        return r["normalWind"] + r["slope"]
    if which == "head":
        return max(rows, key=facing)
    if which == "back":
        return min(rows, key=facing)
    return min(rows, key=lambda r: abs(r["normalWind"]) + abs(r["slope"]))


def model_inputs(h, scheme):
    """The wind and slope getSpeed uses, from the raw values it logged."""
    if scheme == "splines":
        return h["normalWind"], h["slope"]
    s = max(-0.99, min(0.99, h["slope"]))
    c = math.sqrt(1.0 - s * s)
    return h["normalWind"] / c, s / c


def rel(a, b):
    if a == b:
        return 0.0
    return abs(a - b) / max(abs(a), abs(b))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="print every case, not only the mismatches")
    parser.add_argument("--scheme", default="medians",
                        choices=("medians", "weightedMedians", "splines"),
                        help="ForeFire normalScheme")
    parser.add_argument("--probe", help=argparse.SUPPRESS)
    args = parser.parse_args(argv)

    if args.probe:
        print(_SENTINEL + json.dumps(_probe_in_process(json.loads(args.probe))))
        return 0

    cases = [{"fuel": f, "moisture": m, "wind": w, "slope": s,
              "scheme": args.scheme}
             for f in FUELS for m in MOISTURES for w in WINDS for s in SLOPES]
    with ThreadPoolExecutor(max_workers=min(8, os.cpu_count() or 2)) as pool:
        logs = list(pool.map(probe, cases))

    heads = [pick(rows, "head") for rows in logs]
    with tempfile.TemporaryDirectory() as tmp:
        exe = build_reference(tmp)
        model = run_reference(exe, [
            (c["fuel"], MOISTURES[c["moisture"]])
            + model_inputs(h, args.scheme)
            for c, h in zip(cases, heads)])
        pipeline = run_reference(exe, [
            (c["fuel"], MOISTURES[c["moisture"]], c["wind"],
             math.tan(math.radians(c["slope"])))
            for c in cases])

    failures = []
    columns = ("fuel moist    U slope |  R0 ff/ref     phiew ff/ref   "
               "| head ff  model    pipe   | LB   flank ff/ref     back ff/ref")
    print(columns)
    for c, rows, h, mo, pi in zip(cases, logs, heads, model, pipeline):
        flank, back = pick(rows, "flank"), pick(rows, "back")
        errs = {
            "R0": rel(h["rateo"], mo["rateo"]),
            "phiew": rel(h["phiew"], mo["phiew"]),
            "head(model)": rel(h["ROS"], mo["head"]),
            "head(pipeline)": rel(h["ROS"], pi["head"]),
        }
        # The spline normals do not point at the head: report that gap only.
        checked = [k for k in errs
                   if not (k == "head(pipeline)" and args.scheme == "splines")]
        bad = [k for k in checked if errs[k] > REL_TOL]
        if bad:
            failures.append((c, bad))
        if args.verbose or bad:
            print(f"FM{c['fuel']:<3} {c['moisture']:<8} {c['wind']:4.0f} "
                  f"{c['slope']:4.0f}  | {h['rateo']:.4g}/{mo['rateo']:.4g} "
                  f"{h['phiew']:.4g}/{mo['phiew']:.4g} "
                  f"| {h['ROS']:.4g} {mo['head']:.4g} {pi['head']:.4g} "
                  f"| {pi['lb']:.2f} {flank['ROS']:.4g}/{pi['flank']:.4g} "
                  f"{back['ROS']:.4g}/{pi['back']:.4g}"
                  + ("  <- " + ", ".join(bad) if bad else ""))

    sloped = [(c, h, pi) for c, h, pi in zip(cases, heads, pipeline)
              if c["slope"] > 0 and pi["head"] > 0]
    print(f"\n{len(cases)} cases, {len(failures)} failing (rel. tol. {REL_TOL})")
    for deg in sorted({c["slope"] for c, _, _ in sloped}):
        ratios = [h["ROS"] / pi["head"] for c, h, pi in sloped
                  if c["slope"] == deg]
        print(f"slope {deg:.0f} deg, ForeFire head / FARSITE head: "
              f"{min(ratios):.3f} to {max(ratios):.3f}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
