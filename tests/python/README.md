# Python examples and tests

Everything here needs the `pyforefire` module: either a build configured with
`-DFOREFIRE_BUILD_PYTHON=ON`, or `pip install forefire`.

`../run.bash` runs only the first two, through `$PYTHONEXE`.

## `percolation.py`

Four fires side by side, each in an 80×60 band of fuel filled at random to a
different density (`k_coeffs = [.10, .3, .4, .5]`), so the bands sit either
side of the percolation threshold. Writes `percolation.nc`. A demonstration of
building a heterogeneous fuel map in numpy and handing it to ForeFire with
`addIndexLayer`.

## `idealizedwind.py`

One fire under a wind rotating from 0° to 360°, which should trace a circular
front. Writes `360wind.png` through ForeFire's own `plot[]` command — the one
artefact `../run.bash` checks for.

## `farsite_flat.py`

Reproduces a FARSITE benchmark case: flat terrain, a 3 mph north wind, seven
hours of spread, compared against the FARSITE result.

**It cannot run as checked out.** It reads `flatland.lcp`, a landscape file
that is not in this repository. Download it first:

```bash
curl -LO https://github.com/mbedward/farsite/raw/refs/heads/master/examples/flatland/Inputs/a_lcpFiles/flatland.lcp
```

The weather it uses, `flatland_3mph0deg7hr.raws`, *is* in this directory.

## The test scripts

These three are tests rather than examples, and `../run.bash` runs none of
them. They are plain scripts, not pytest modules. `TESTING.md` covers each in
detail, including how to build the module they import.

### `test_moisture_invariants.py`

Asserts, for every propagation model that consumes dead fuel moisture, that
rate of spread stays finite, falls as moisture rises, reaches zero at the
moisture of extinction, and responds to a dynamic moisture layer. It holds no
reference data — every assertion follows from the published spread equations —
so unlike `runff` it can tell a physics fix from a physics regression.

Run in CI by `invariants.yml`. Manually:

```bash
python tests/python/test_moisture_invariants.py        # -v for every probe
```

`--model NAME` and `--test NAME`, both repeatable, narrow it down.

### `test_threading.py`

Runs eight simulations in threads and requires each to reproduce the node
count it produces alone.

**It does not pass, by design**, and is not run in CI. It is the failing
reproduction for the shared-state problem in
[#175](https://github.com/forefireAPI/forefire/issues/175). It needs a
free-threaded CPython and `PYTHON_GIL=0`; on an ordinary interpreter the GIL
serialises every call and the test skips, reporting a green result that proves
nothing.

```bash
PYTHON_GIL=0 python3.14t tests/python/test_threading.py
```

### `test_wheel.py`

The smoke test `cibuildwheel` runs against a built wheel: the module imports,
its vendored NetCDF resolves, the propagation models registered, and a trivial
simulation advances. Run it against an *installed* `forefire`, never from the
source tree:

```bash
python tests/python/test_wheel.py
```
