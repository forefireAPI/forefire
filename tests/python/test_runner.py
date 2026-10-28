"""Tests for `pyforefire.ForeFireRunner`, one simulation per worker process.

Unlike `test_threading.py`, this needs no free-threaded interpreter: each
runner is its own process, so threads driving separate runners overlap even
with the GIL on.

    python tests/python/test_runner.py

Exit status is 0 only if every check passes.
"""

import os
import signal
import sys
import threading
import traceback

SIZE = 1000
STEPS = 5
RUNNERS = 8


def run_simulation(ff):
    """One self-contained simulation. Returns its fire node count."""
    ff.execute(f"FireDomain[sw=(0,0,0);ne=({SIZE},{SIZE},0);t=0]")
    ff.addLayer("propagation", "Iso", "propagationModel")
    ff.execute(f"startFire[loc=({SIZE / 2},{SIZE / 2},0)]")
    for _ in range(STEPS):
        ff.execute("step[dt=100]")
    return ff.execute("print[]").count("FireNode")


def test_matches_in_process(baseline):
    """A runner must give exactly what the in-process object gives."""
    import pyforefire

    with pyforefire.ForeFireRunner() as runner:
        got = run_simulation(runner)
    if got != baseline:
        return [f"runner produced {got} fire nodes, expected {baseline}"]
    return []


def test_concurrent_runners(baseline):
    """Runners driven from separate threads must not disturb each other."""
    import pyforefire

    results = [None] * RUNNERS
    errors = [None] * RUNNERS

    def worker(i):
        try:
            with pyforefire.ForeFireRunner() as runner:
                results[i] = run_simulation(runner)
        except BaseException:
            errors[i] = traceback.format_exc()

    threads = [threading.Thread(target=worker, args=(i,)) for i in range(RUNNERS)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()

    failures = []
    for i, err in enumerate(errors):
        if err is not None:
            failures.append(f"runner {i} raised:\n{err}")
    for i, got in enumerate(results):
        if errors[i] is None and got != baseline:
            failures.append(f"runner {i} produced {got} fire nodes, expected {baseline}")
    return failures


def test_parameters():
    """`runner[key]` reads and writes the worker's parameters."""
    import pyforefire

    failures = []
    with pyforefire.ForeFireRunner() as runner:
        runner["perimeterResolution"] = 7.5
        if runner["perimeterResolution"] != 7.5:
            failures.append(f"read back {runner['perimeterResolution']!r}, expected 7.5")
        runner.setString("outputsFormat", "json")
        if runner.getString("outputsFormat") != "json":
            failures.append("setString/getString did not round-trip")
    return failures


def test_errors_are_raised_here():
    """Exceptions in the worker reach the caller, and the worker survives them."""
    import pyforefire

    failures = []
    with pyforefire.ForeFireRunner() as runner:
        # Without a domain, reading a missing key segfaults instead of raising.
        runner.execute(f"FireDomain[sw=(0,0,0);ne=({SIZE},{SIZE},0);t=0]")
        try:
            runner.noSuchMethod()
            failures.append("calling a missing method did not raise")
        except AttributeError:
            pass
        try:
            runner["noSuchParameter"]
            failures.append("reading a missing parameter did not raise")
        except RuntimeError:
            pass
        runner["perimeterResolution"] = 3.0
        if runner["perimeterResolution"] != 3.0:
            failures.append("the worker stopped answering after an error")
    return failures


def test_worker_death():
    """A dead worker raises WorkerDied instead of taking the caller down."""
    import pyforefire

    runner = pyforefire.ForeFireRunner()
    os.kill(runner._process.pid, signal.SIGKILL)
    runner._process.wait()
    failures = []
    for attempt in ("first", "second"):
        try:
            runner.execute("print[]")
            failures.append(f"the {attempt} call on a dead worker did not raise")
        except pyforefire.WorkerDied:
            pass
    runner.close()
    return failures


def main():
    import pyforefire

    baseline = run_simulation(pyforefire.ForeFire())
    print(f"in-process baseline: {baseline} fire nodes")

    failures = []
    failures += test_matches_in_process(baseline)
    failures += test_concurrent_runners(baseline)
    failures += test_parameters()
    failures += test_errors_are_raised_here()
    failures += test_worker_death()

    if failures:
        print(f"\nFAILED with {len(failures)} problem(s):")
        for f in failures:
            print(f"  - {f}")
        return 1
    print(f"OK: {RUNNERS} concurrent runners matched the baseline; errors and crashes surfaced")
    return 0


if __name__ == "__main__":
    sys.exit(main())
