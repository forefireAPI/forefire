"""Tests for `pyforefire.run_ensemble` and `pyforefire.burn_probability`.

    python tests/python/test_ensemble.py

Exit status is 0 only if every check passes.
"""

import os
import sys
import tempfile

import numpy as np

SIZE = 1000
UNTIL = 500


def setup(ff):
    ff.execute(f"FireDomain[sw=(0,0,0);ne=({SIZE},{SIZE},0);t=0]")
    ff.addLayer("propagation", "Iso", "propagationModel")
    ff.execute(f"startFire[loc=({SIZE / 2},{SIZE / 2},0)]")
    ff.execute("goTo[t=100]")


def deterministic():
    """The arrival times of one in-process run of `setup`."""
    import pyforefire

    ff = pyforefire.ForeFire()
    setup(ff)
    ff.execute(f"goTo[t={UNTIL}]")
    with tempfile.TemporaryDirectory() as tmp:
        return pyforefire.arrival_time(ff, os.path.join(tmp, "at.asc"))


def test_identical_members(reference):
    """Unperturbed members, forked or not, all reproduce the deterministic run."""
    import pyforefire

    failures = []
    for fork in (True, False):
        arrivals = pyforefire.run_ensemble(setup, [{}] * 3, UNTIL, fork=fork)
        if arrivals.shape != (3,) + reference.shape:
            failures.append(f"fork={fork}: shape {arrivals.shape}, expected {(3,) + reference.shape}")
            continue
        if not all(np.array_equal(a, reference) for a in arrivals):
            failures.append(f"fork={fork}: a member differs from the deterministic run")
        p = pyforefire.burn_probability(arrivals)
        if not np.array_equal(p, np.isfinite(reference).astype(float)):
            failures.append(f"fork={fork}: probability is not the deterministic burnt area")
    return failures


def test_forked_perturbation():
    """`perturb` runs on each fork, after the shared setup."""
    import pyforefire

    def ignite(ff, loc):
        if loc is not None:
            ff.execute(f"startFire[loc=({loc[0]},{loc[1]},0)]")

    # By t=250 the shared fire is still far from the corner.
    corner = (150, 150)
    arrivals = pyforefire.run_ensemble(setup, [None, corner, corner, corner], 250, perturb=ignite)
    p = pyforefire.burn_probability(arrivals)
    n = arrivals.shape[1]
    i = int(corner[1] / SIZE * n)
    j = int(corner[0] / SIZE * n)
    failures = []
    if p[i, j] != 0.75:
        failures.append(f"probability {p[i, j]} at the second ignition, expected 0.75")
    if p[n // 2, n // 2] != 1.0:
        failures.append(f"probability {p[n // 2, n // 2]} at the shared ignition, expected 1")
    return failures


def test_construction_parameters():
    """With fork=False, parameters read when the model is built take effect."""
    import pyforefire

    speeds = [0.5, 1.0, 2.0]
    arrivals = pyforefire.run_ensemble(setup, [{"Iso.speed": s} for s in speeds], UNTIL, fork=False)
    burnt = np.isfinite(arrivals).sum(axis=(1, 2))
    if not (burnt[0] < burnt[1] < burnt[2]):
        return [f"burnt cells {list(burnt)} do not grow with Iso.speed {speeds}"]
    return []


def test_probability_properties():
    """Probabilities lie in [0, 1] and never decrease with time."""
    import pyforefire

    arrivals = pyforefire.run_ensemble(setup, [{"Iso.speed": s} for s in (0.5, 1, 1.5, 2)], UNTIL, fork=False)
    failures = []
    previous = None
    for t in (150, 250, 350, UNTIL):
        p = pyforefire.burn_probability(arrivals, t)
        if p.min() < 0 or p.max() > 1:
            failures.append(f"probability outside [0, 1] at t={t}")
        if previous is not None and (p < previous).any():
            failures.append(f"probability decreased somewhere between two times up to t={t}")
        previous = p
    if not np.array_equal(pyforefire.burn_probability(arrivals, UNTIL), pyforefire.burn_probability(arrivals)):
        failures.append("the probability at `until` differs from the probability of burning at all")
    return failures


def test_member_error():
    """An exception in one member reaches the caller."""
    import pyforefire

    def fail(ff, member):
        if member:
            raise ValueError("member failed")

    try:
        pyforefire.run_ensemble(setup, [False, True], UNTIL, perturb=fail)
    except ValueError:
        return []
    return ["a failing member did not raise"]


def main():
    reference = deterministic()
    print(f"deterministic run: {int(np.isfinite(reference).sum())} burnt cells of {reference.size}")

    failures = []
    failures += test_identical_members(reference)
    failures += test_forked_perturbation()
    failures += test_construction_parameters()
    failures += test_probability_properties()
    failures += test_member_error()

    if failures:
        print(f"\nFAILED with {len(failures)} problem(s):")
        for f in failures:
            print(f"  - {f}")
        return 1
    print("OK: ensembles reproduce the deterministic run, perturbations apply, probabilities are consistent")
    return 0


if __name__ == "__main__":
    sys.exit(main())
