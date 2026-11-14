# Testing ForeFire

All tests live in `tests/`.

| Suite | What it checks | CI |
| --- | --- | --- |
| C++ unit tests (`tests/unit`) | propagation and flux models, one call at a time | `main.yml`, `macos.yml`, `sanitizers.yml` |
| `tests/runff` | a real case, reloaded, against `.ref` files | `main.yml`, `macos.yml`, `sanitizers.yml`, `docker.yml` |
| `tests/runANN` | the ANN propagation model | `main.yml` |
| `test_moisture_invariants.py` | spread equations, with no reference data | `invariants.yml` |
| `test_runner.py` | `ForeFireRunner` workers and `fork()` | `invariants.yml` |
| `test_ensemble.py` | `run_ensemble` and `burn_probability` | `invariants.yml` |
| `test_wheel.py` | an installed wheel | `wheels.yml` |
| `test_genforefirecase.py` | the landscape generator, read back by ForeFire | no |
| `test_validate.py` | the `forefire-validate` checker | no, `tests/run.bash` |
| `test_farsite_reference.py` | Farsite against `mbedward/farsite` | no |
| `test_threading.py` | threads on one interpreter; fails today | no |

## Writing a test

Before writing a test, name the bug it catches. If you cannot, do not write
it. Never regenerate a `.ref` file to make a test pass.

## Unit tests

```bash
cmake -S . -B build && cmake --build build -j
ctest --test-dir build --output-on-failure
```

No Python and no test data. `-DFOREFIRE_BUILD_TESTS=OFF` skips them.
`tests/unit/README.md` explains how to add one.

## runff

```bash
git lfs pull --include tests/runff
cd tests/runff && bash ff-run.bash
```

Needs `pip install lxml xarray netCDF4`. It runs `real_case.ff`, reloads it
with `reload_case.ff` and compares the NetCDF and KML output with the `.ref`
files, within a numerical tolerance. Configure checks the LFS files;
`-DFOREFIRE_CHECK_LFS=OFF` skips that check when only `tests/runff` was
pulled. If you regenerate a `.ref` on purpose, update its md5 in
`scripts/check_all_lfs.bash`.

runff compares ForeFire with frozen ForeFire output, so it cannot tell a
physics fix from a regression. The moisture invariants can.

## Python tests

```bash
python3 -m venv .venv
./.venv/bin/python -m pip install .
./.venv/bin/python tests/python/test_moisture_invariants.py
```

The others run the same way. Notes:

- **Invariants:** for every model that reads dead fuel moisture, the rate of
  spread stays finite, falls as moisture rises and reaches zero at extinction.
  `--model NAME` and `--test NAME` narrow it; `-v` prints every probe.
- **Global state:** the core keeps it, so a second `ForeFire()` in one process
  inherits the first one's parameters, and a parameter sweep silently returns
  the same result. Each invariant probe runs in its own interpreter; use
  `ForeFireRunner` in any new test that varies parameters.
- **Threading:** needs free-threaded CPython and `PYTHON_GIL=0`
  (`PYTHON_GIL=0 python3.14t tests/python/test_threading.py`). It segfaults or
  returns wrong counts today and is kept as the reproduction for running
  simulations in threads.
- **Farsite reference:** needs a C++ compiler. Flank and back are reported, not
  asserted. `--scheme splines` uses that scheme.
- `percolation.py`, `idealizedwind.py` and `farsite_flat.py` are examples
  that plot and assert nothing.

## runANN

```bash
cd tests/runANN && bash run.bash
```

Needs `-DFOREFIRE_BUILD_TOOLS=ON` (the default outside wheels). It runs
`Rothermel.ffann` over `modelrun.csv` and fails above an RMSE of 0.05; the
trained network scores 0.0235 and predicting the mean scores 0.0966. The
fixture is weak: its outputs take only four values.

## Everything at once

`tests/run.bash` runs `runff` and `runANN`, the `mnh_*` coupled cases when
`SRC_MESONH` is set, and `tests/python/run.bash` when `PYTHONEXE` is set.
CI does not use it.

## Sanitizers

```bash
cmake -S . -B build-asan -DFOREFIRE_SANITIZE=address
cmake --build build-asan -j
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build-asan --output-on-failure
```

This builds with `-g -O1` and writes `bin/` and `lib/` like any build, so it
replaces a release build in the source tree. CI runs `address` on the unit
tests and runff: any error blocks, leaks block on the unit tests only
(runff still leaks about 200 kB). Other sanitizers pass through but are not
verified clean.

## Compiler warnings

`-Wall -Wextra` is on and not clean, so warnings do not fail the build.
`-DFOREFIRE_ENABLE_WARNINGS=OFF` silences them;
`-DFOREFIRE_WARNINGS_AS_ERRORS=ON` makes them fatal.


## FFState and Scenario Studio

FFState object/event/arrival restoration is covered by `unit.ffstate` in CTest.
Fresh-process checks and the opt-in MPI/Meso-NH driver are documented in
[tests/ffstate/README.md](tests/ffstate/README.md). Coupled tests are not CI and
must not be inferred from browser or standalone test results.

After building the WASM module:

```sh
node bindings/wasm/smoke.mjs bin/forefire.mjs
python3 bindings/wasm/scenario-studio/prepare.py
node --test local/scenario-studio/tests/*.test.mjs
FFTILE_FIXTURES="$PWD/test_output_tiles" node --test local/scenario-studio/tests/*.test.mjs
```

No tile download is needed for staging or the first Node suite. Real tile tests
skip without `FFTILE_FIXTURES`; the second invocation enables them. Do not count
skips as passes. The tests catch false future burn at startup, shared initial
burn colouring, slower/faster ensemble bias, separate-worker isolation and
scenario resets, alongside tile and model checks. Browser/Cesium/WebCodecs
rendering still needs browser QA.

The Pages artifact alone downloads Corsica tiles with `prepare.py --corsica`;
these tiles, generated binaries and credentials never enter the source commit.
