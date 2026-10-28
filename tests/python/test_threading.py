"""Concurrency stress test for running several simulations in one process.

ForeFire keeps simulation state in process-wide statics, so two domains
running at the same time can corrupt each other. Under a normal CPython the
GIL serialises every call into the extension and hides all of it, which is why
this test only means anything on a free-threaded interpreter with the GIL
actually off:

    PYTHON_GIL=0 python3.14t tests/python/test_threading.py

`PYTHON_GIL=0` is needed because `_pyforefire` does not declare
`py::mod_gil_not_used()`, so importing it would otherwise switch the GIL back
on and the test would pass without proving anything.

Exit status is 0 only if every thread produced exactly the result it produces
when run alone.
"""

import os
import sys
import threading
import traceback

SIZE = 1000
STEPS = 5
THREADS = 8


def gil_enabled():
    """True if the GIL is active, None if this interpreter has no switch."""
    if not hasattr(sys, "_is_gil_enabled"):
        return None
    return sys._is_gil_enabled()


def run_simulation(seed):
    """One self-contained simulation. Returns its fire node count."""
    import pyforefire

    ff = pyforefire.ForeFire()
    ff.execute(f"FireDomain[sw=(0,0,0);ne=({SIZE},{SIZE},0);t=0]")
    ff.addLayer("propagation", "Iso", "propagationModel")
    ff.execute(f"startFire[loc=({SIZE / 2},{SIZE / 2},0)]")
    for _ in range(STEPS):
        ff.execute("step[dt=100]")
    return ff.execute("print[]").count("FireNode")


def test_concurrent_domains(baseline):
    """Every thread must reproduce the single-threaded result exactly."""
    results = [None] * THREADS
    errors = [None] * THREADS

    def worker(i):
        try:
            results[i] = run_simulation(i)
        except BaseException:
            errors[i] = traceback.format_exc()

    threads = [threading.Thread(target=worker, args=(i,)) for i in range(THREADS)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()

    failures = []
    for i, err in enumerate(errors):
        if err is not None:
            failures.append(f"thread {i} raised:\n{err}")
    for i, got in enumerate(results):
        if errors[i] is None and got != baseline:
            failures.append(f"thread {i} produced {got} fire nodes, expected {baseline}")
    return failures


def test_concurrent_construction():
    """Hammer object creation, which draws IDs from a shared counter.

    Every ForeFireAtom takes its id from `instanceNRCount++`, which is not
    atomic, so concurrent construction can hand the same id to two objects.
    """
    import pyforefire

    made = []
    lock = threading.Lock()
    errors = []

    def worker():
        try:
            local = [pyforefire.ForeFire() for _ in range(25)]
            with lock:
                made.extend(local)
        except BaseException:
            with lock:
                errors.append(traceback.format_exc())

    threads = [threading.Thread(target=worker) for _ in range(THREADS)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()

    if errors:
        return [f"construction raised:\n{errors[0]}"]
    expected = THREADS * 25
    if len(made) != expected:
        return [f"created {len(made)} objects, expected {expected}"]
    return []


def main():
    # Import before reading the GIL state. An extension that does not declare
    # `py::mod_gil_not_used()` switches the GIL back on as it is imported, so
    # checking beforehand reports the interpreter's default rather than the
    # state this test actually runs under.
    before = gil_enabled()
    import pyforefire  # noqa: F401

    gil = gil_enabled()
    print(f"interpreter: {sys.version.split()[0]}")
    print(f"GIL enabled: {gil} (was {before} before importing pyforefire)")
    if gil is None:
        print("SKIP: not a free-threaded interpreter, this test cannot prove anything")
        return 0
    if gil:
        print(
            "SKIP: the GIL is on, so every call is serialised and races stay hidden.\n"
            "      Re-run with PYTHON_GIL=0 to actually exercise concurrency."
        )
        return 0

    baseline = run_simulation(0)
    print(f"single-threaded baseline: {baseline} fire nodes")

    failures = []
    failures += test_concurrent_domains(baseline)
    failures += test_concurrent_construction()

    if failures:
        print(f"\nFAILED with {len(failures)} problem(s):")
        for f in failures[:10]:
            print(f"  - {f}")
        return 1
    print(f"\nOK: {THREADS} concurrent simulations all matched the baseline")
    return 0


if __name__ == "__main__":
    sys.exit(main())
