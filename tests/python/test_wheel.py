"""Smoke test for the built `forefire` wheel.

Run against an *installed* wheel (never from the source tree), so it only
touches things a user gets from `pip install forefire`: the extension module
loads, its vendored NetCDF dependency resolves, and a trivial simulation
actually advances.

    python tests/python/test_wheel.py
"""

import contextlib
import os
import sys
import tempfile


class CapturedOutput:
    """Text the C++ side wrote to stdout, which `sys.stdout` never sees.

    ForeFire reports unknown models on `std::cout`, so the capture has to
    happen at the file-descriptor level.
    """

    def __init__(self):
        self.text = ""


@contextlib.contextmanager
def captured_native_stdout():
    captured = CapturedOutput()
    with tempfile.TemporaryFile(mode="w+") as tmp:
        sys.stdout.flush()
        saved = os.dup(1)
        try:
            os.dup2(tmp.fileno(), 1)
            yield captured
        finally:
            os.dup2(saved, 1)
            os.close(saved)
        tmp.seek(0)
        captured.text = tmp.read()


def test_import():
    import pyforefire

    print("pyforefire imported from", pyforefire.__file__)
    assert hasattr(pyforefire, "ForeFire"), "ForeFire class missing from the module"
    return pyforefire


def test_no_domain(pyforefire):
    """Calls that need a domain must raise before FireDomain[...], not segfault.

    Runs before test_simulation: the domain is process-wide, so once one
    exists a new ForeFire() sees it too.
    """
    import numpy as np

    ff = pyforefire.ForeFire()
    calls = {
        "ff[key]": lambda: ff["noSuchKey"],
        "addLayer": lambda: ff.addLayer("propagation", "Iso", "propagationModel"),
        "addScalarLayer": lambda: ff.addScalarLayer(
            "data", "windU", 0, 0, 0, 10, 10, 0, np.zeros((1, 1, 2, 2))
        ),
        "addIndexLayer": lambda: ff.addIndexLayer(
            "table", "fuel", 0, 0, 0, 10, 10, 0, np.zeros((1, 1, 2, 2), dtype=np.int32)
        ),
        "getDoubleArray": lambda: ff.getDoubleArray("windU"),
    }
    for name, call in calls.items():
        try:
            call()
        except RuntimeError as e:
            assert "FireDomain" in str(e), f"{name} raised an unrelated error: {e}"
        else:
            raise AssertionError(f"{name} did not raise without a FireDomain")
    print(f"{len(calls)} calls raised cleanly without a FireDomain")


def test_simulation(pyforefire):
    """Burn an isotropic circle and check the front actually grew."""
    ff = pyforefire.ForeFire()
    size = 10000

    ff.execute(f"FireDomain[sw=(0,0,0);ne=({size},{size},0);t=0]")

    # Propagation models register themselves from static initialisers, so a
    # build that drops those objects leaves the model table empty and the
    # simulation segfaults a few calls later. Fail here instead, with a
    # readable message.
    with captured_native_stdout() as captured:
        ff.addLayer("propagation", "Iso", "propagationModel")
    assert "not recognized" not in captured.text, (
        "propagation model 'Iso' is not registered — the core was probably "
        f"linked in a way that discards the model objects:\n{captured.text}"
    )

    ff.execute(f"startFire[loc=({size / 2},{size / 2},0.0)]")
    ff.execute("step[dt=1000]")

    front = ff.execute("print[]")
    assert "FireNode" in front, f"no fire nodes after 1000 s of simulation:\n{front}"
    print(f"simulation produced {front.count('FireNode')} fire nodes")


def test_helpers():
    """`pyforefire.helpers` pulls in numpy/matplotlib, which are hard deps."""
    from pyforefire import helpers

    table = helpers.get_fuels_table("Rothermel")()
    assert table.startswith("Index;Rhod"), f"unexpected fuel table header: {table[:40]!r}"
    print(f"fuel table loaded with {table.count(chr(10))} rows")


def main():
    pyforefire = test_import()
    test_no_domain(pyforefire)
    test_simulation(pyforefire)
    test_helpers()
    print("\nOK: forefire wheel smoke test passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
