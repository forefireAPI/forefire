"""Run an ensemble of simulations and turn it into a burn-probability map.

Each member runs in its own `ForeFireRunner`, so members run in parallel and
cannot disturb each other (see issue #172).

    def setup(ff):
        ff.execute("FireDomain[sw=(0,0,0);ne=(1000,1000,0);t=0]")
        ff.addLayer("propagation", "Iso", "propagationModel")
        ff.execute("startFire[loc=(500,500,0)]")
        ff.execute("goTo[t=600]")  # spin-up shared by every member

    def perturb(ff, wind):
        ff.execute(f"trigger[wind;vel=({wind[0]},{wind[1]},0)]")

    arrivals = run_ensemble(setup, [(3, 0), (2, 2), (0, 3)], until=3600, perturb=perturb)
    probability = burn_probability(arrivals, t=1800)
"""

import os
import subprocess
import tempfile
from concurrent.futures import ThreadPoolExecutor

import numpy as np

from ._runner import ForeFireRunner

__all__ = ["run_ensemble", "burn_probability", "arrival_time"]


def arrival_time(ff, path, bbox=None):
    """Return the arrival-time map of `ff` as an array indexed [y, x].

    Row 0 is the south edge, and cells the front has not reached are `inf`.
    `bbox=(left, bottom, right, top)` in domain coordinates crops the map to
    that area at the same resolution; by default it covers the whole domain.
    The map is written to `path` by the kernel's `plot` command, as an ASCII
    grid with one row per x, then read back.
    """
    area = "" if bbox is None else ";BBoxLBRT=({},{},{},{})".format(*bbox)
    ff.execute(f"plot[parameter=arrival_time;filename={path}{area}]")
    return np.loadtxt(path, skiprows=6, ndmin=2).T


def _set_parameters(ff, member):
    for key, value in member.items():
        ff[key] = value


def run_ensemble(setup, members, until, perturb=_set_parameters, fork=True,
                 bbox=None, max_workers=None, stdout=subprocess.DEVNULL, stderr=None):
    """Run one simulation per item of `members` up to time `until`.

    `setup(ff)` builds the simulation: domain, layers, ignition and any
    spin-up steps. `perturb(ff, member)` applies one member's changes; by
    default `member` is a dict of parameters, set with `ff[key] = value`.

    With `fork=True`, `setup` runs once and each member is a fork of it, so
    `perturb` runs after `setup`. It can change the wind with
    `trigger[wind;vel=(u,v,0)]`, the fuels with `trigger[fuel;...]`, or any
    parameter the kernel reads while stepping. With `fork=False`, each member
    gets a fresh runner and `perturb` runs before `setup`, so it can also set
    the parameters read only when the domain and models are built, such as
    `windReductionFactor` and `propagationSpeedAdjustmentFactor`.

    `bbox=(left, bottom, right, top)` limits the maps to that area (see
    `arrival_time`). The result holds one float64 map per member, so crop a
    large domain to where the fire can go.

    Members run `max_workers` at a time, one per CPU by default. The kernel's
    output is discarded unless `stdout` and `stderr` say otherwise (see
    `ForeFireRunner`).

    Returns the arrival times as an array indexed [member, y, x], `inf` where
    a member's front did not arrive by `until`.
    """
    members = list(members)
    if max_workers is None:
        max_workers = os.cpu_count() or 1

    base = None

    def run(index, member):
        ff = base.fork() if fork else ForeFireRunner(stdout=stdout, stderr=stderr)
        with ff:
            perturb(ff, member)
            if not fork:
                setup(ff)
            ff.execute(f"goTo[t={until}]")
            return arrival_time(ff, os.path.join(tmp, f"member{index}.asc"), bbox)

    with tempfile.TemporaryDirectory(prefix="forefire-ensemble-") as tmp:
        try:
            if fork:
                base = ForeFireRunner(stdout=stdout, stderr=stderr)
                setup(base)
            with ThreadPoolExecutor(max_workers=max_workers) as pool:
                maps = list(pool.map(run, range(len(members)), members))
        finally:
            if base is not None:
                base.close()
    return np.stack(maps) if maps else np.empty((0, 0, 0))


def burn_probability(arrivals, t=None):
    """Return the fraction of members burnt at each point by time `t`.

    `arrivals` is the [member, y, x] array from `run_ensemble`. With `t`
    None, a point counts as burnt if the front reached it at all.
    """
    arrivals = np.asarray(arrivals)
    burnt = np.isfinite(arrivals) if t is None else arrivals <= t
    return burnt.mean(axis=0)
