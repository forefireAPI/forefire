# ForeFire test suite

Six sets of tests, one per directory, covering the different interfaces and
use-cases of ForeFire.

| Directory | What it covers | Needs |
| --- | --- | --- |
| `unit` | C++ tests of one model at a time, without running a simulation, covering every registered propagation and flux model | Nothing beyond the build; see `unit/README.md` |
| `mnh_ideal` | ForeFire / **Meso-NH** coupling on an idealised atmospheric case | `SRC_MESONH` set; Meso-NH compiled with the ForeFire library in its `exe` directory |
| `mnh_real_nested` | ForeFire / **Meso-NH** coupling on a real nested case | Same as above |
| `python` | The Python bindings, through two example simulations | `PYTHONEXE` set to a Python interpreter that can `import pyforefire` |
| `runANN` | The built-in feed-forward network evaluator, on a network fitted to Rothermel | `bin/ANN_test`, built by default (`-DFOREFIRE_BUILD_TOOLS=ON`) |
| `runff` | The command-line interpreter: run a case, save and reload state, export KML and GeoJSON | ForeFire only |

`TESTING.md` at the repository root describes each of these in more detail,
along with the sanitizer build.

The `unit` suite is not run by `run.bash` below — it is a CTest suite, built
with everything else:

```bash
cmake -S . -B build && cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Prerequisites

* **ForeFire built**, with the binaries in `bin/`.

* The test fixtures are stored in **Git LFS**. `git lfs pull` if the `.nc`
  files look like short text stubs.

* *(optional)* **Meso-NH**, for the `mnh_*` tests only:

  ```bash
  export SRC_MESONH=/path/to/your/mesonh
  ```

* *(optional)* **A Python interpreter with the bindings available**, for the
  `python` tests only. This is either a build with
  `-DFOREFIRE_BUILD_PYTHON=ON`, or an environment where `pip install forefire`
  has been run:

  ```bash
  export PYTHONEXE=/path/to/your/python
  ```

* The verification scripts under `runff` import `lxml`, `xarray` and
  `netCDF4`:

  ```bash
  pip install lxml xarray netCDF4
  ```

## Running them

```bash
cd tests
bash run.bash     # or: make test
```

`run.bash` skips the `mnh_*` tests when `SRC_MESONH` is unset and the `python`
tests when `PYTHONEXE` is unset, runs `runff` and `runANN` unconditionally, and
prints a pass/fail summary. It exits non-zero if any suite failed.

To clean up the outputs (ForeFire dumps, figures, NetCDF files):

```bash
bash clean.bash   # or: make clean
```

## What each set does

### `mnh_ideal`

Validates the coupling on a simplified atmospheric profile. Expect a fire front
consistent with the prescribed wind, plus NetCDF and KML outputs.

### `mnh_real_nested`

Reproduces a real fire scenario across two nested Meso-NH domains (real forcing
plus a high-resolution nest). It also exercises high-frequency output and the
HTTP web interface, which needs `FOREFIREHOME` set.

### `python`

`run.bash` runs two scripts with `$PYTHONEXE`:

| Script | What it does | Output |
| --- | --- | --- |
| `percolation.py` | Four fires, each in a band of randomly filled fuel at a different density | `percolation.nc` |
| `idealizedwind.py` | Wind rotating from 0° to 360°, giving a circular front | `360wind.png`, written by ForeFire's own `plot[]` command |

The only assertion is that `360wind.png` was produced and is not empty — these
are demonstrations of the API rather than tests of the physics.

`farsite_flat.py` is in this directory but is **not** run by `run.bash`. It
compares ForeFire against a FARSITE case, and needs `flatland.lcp`, which is
not in the repository — `python/README.md` has the download URL.

Three test scripts sit here too, and `run.bash` runs none of them:

| File | What it checks | Where it runs |
| --- | --- | --- |
| `test_moisture_invariants.py` | That rate of spread responds to dead fuel moisture the way the spread equations require, for every model that reads it | CI, `invariants.yml` |
| `test_threading.py` | That eight concurrent simulations each reproduce the result they give alone | By hand only — it is the failing reproduction for [#175](https://github.com/forefireAPI/forefire/issues/175), and needs a free-threaded CPython |
| `test_wheel.py` | The cibuildwheel smoke test: the module imports, its vendored NetCDF resolves, the models registered, a trivial simulation advances | cibuildwheel, against an *installed* wheel |

`TESTING.md` gives the command line for each.

### `runANN`

Loads `Rothermel.ffann` — a small network fitted to the Rothermel propagation
model — evaluates it over the fuel, slope and wind combinations in
`modelrun.csv`, and checks the result.

It needs no machine-learning framework. `ANN_test` is a ForeFire tool built
from `tools/runANN/ANNTest.cpp`, and the `.ffann` format is read by ForeFire's
own evaluator.

The check is the root mean squared error between what the network predicts and
what the propagation model produced, against a tolerance of 0.05. That
tolerance sits between the trained network (about 0.024) and a predictor that
ignores its inputs and returns the mean (0.097), so a network that stopped
working would fail. It needs no reference file — the earlier version diffed
against a `result.txt.ref` that was never committed, and so failed on its
second line every time it ran.

### `runff`

Exercises the command-line interpreter. There are two entry points, and they do
different things:

* **`run.bash`** — what `tests/run.bash` calls. Three successive scenarios:

  1. `real_case.ff` — run a real case, write NetCDF output and a `to_reload.ff`
     state file;
  2. `reload_case.ff` — reload that state and export KML;
  3. `rungeojson.ff` — load, simulate, export GeoJSON, clear memory, reload the
     GeoJSON to verify it.

  It then checks that the expected artefacts exist and are not implausibly
  small.

* **`ff-run.bash`** — what CI calls, from `main.yml`, `macos.yml` and
  `docker.yml`. It runs the first two scenarios and then compares the KML and
  NetCDF against `real_case.kml.ref` and `ForeFire.0.nc.ref` with a numerical
  tolerance, using `compare_kml.py` and `compare_nc.py`. This is the one that
  can detect physics drift.
