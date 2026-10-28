# Unit tests

C++ tests that exercise one model at a time, without running a simulation.

`tests/runff` runs a whole case and diffs the output against reference files.
That catches physics drift along the path it takes — but it takes one path,
through one propagation model (`Rothermel`, set in `params.ff`). Nothing was
watching the other eighteen propagation models or any of the sixteen flux
models. These tests are for them.

## Running them

They build with everything else, and are registered with CTest:

```bash
cmake -S . -B build && cmake --build build -j
ctest --test-dir build --output-on-failure
```

Or run the binary directly, which gives finer control:

```bash
./bin/forefire_unit_tests                          # everything
./bin/forefire_unit_tests --test-suite="flux models"
./bin/forefire_unit_tests --test-case="*Rothermel*"
./bin/forefire_unit_tests --list-test-cases
```

`-DFOREFIRE_BUILD_TESTS=OFF` skips building them. Wheel builds default to off.

The framework is [doctest](https://github.com/doctest/doctest) 2.5.3, vendored
as a single header in `third_party/doctest/`. See that directory's README.

## How a model gets tested

`getSpeedForNode` splits in two: the DataBroker gathers properties out of the
simulation into a `double*`, and then the model does arithmetic on that array.
Only the second half is physics, and it needs nothing but the array — so
`ModelSandbox` builds an empty `FireDomain` purely to instantiate models, and
`Inputs` fills their property array by property *name* rather than by index.

Addressing by name matters. A model's properties are numbered in the order its
constructor calls `registerProperty`, so a test written against raw indices
would keep passing after someone reorders that constructor, while silently
testing a different quantity.

Parameters are process-global — every `SimulationParameters` method reads and
writes `GetInstance()` whatever instance it is called on — so a test setting
`Iso.speed`, or `burningDuration` to zero, would otherwise change the result of
whichever test ran next. `ModelSandbox` snapshots the parameter map on
construction and puts it back on destruction.

That restore is load-bearing rather than precautionary: with it removed, three
of five `--order-by=rand` seeds fail. Running the suite under a few seeds is a
cheap way to check it still holds:

```bash
./bin/forefire_unit_tests --order-by=rand --rand-seed=1337
```

## What the assertions mean

Two kinds, and they are not equally trustworthy.

**Invariants** — no spread without fuel, more wind never means less spread,
a downslope is not an upslope, total released energy does not depend on how
the time window is cut. These should hold whatever the implementation is, and
they are the ones worth trusting.

**Pinned values** — `recorded rates of spread for the standard fuel` is a
record of what ForeFire produces today, at a 1e-5 relative tolerance so that
`-march=native` and floating-point contraction differences between CI runners
do not trip it. A pin moving means the model changed; that may well be
intended, but it should be a decision rather than a surprise. The pins carry
no claim of matching published values.

## Things found while writing these

**Models are never destroyed in a normal run.** `FireDomain` keeps them in
`propModelsTable` and `fluxModelsTable` and frees neither, so every model a
simulation instantiates is leaked. That is why the two memory bugs below went
unnoticed for so long: the code that trips them does not otherwise run. Still
open.

**The `properties` array was deleted twice** — fixed, along with the test that
holds it fixed. Sixteen flux models and two propagation models deleted
`properties` in their own destructor while `~ForeFireModel` deleted it again,
and the flux ones used scalar `delete` on an array allocated with `new[]`. It is
allocated by each model's constructor and freed by the base class, so the
derived deletes are simply gone. `every model can be destroyed` covers all 33
models; reintroduce one delete and it aborts with `double free or corruption`.

`~ForeFireModel` itself left `properties` uninitialised, so destroying a model
that registers *no* properties — `Iso`, `heatFluxBasic` — deleted whatever the
member happened to be built over. It also freed `fuelPropertiesTable`, allocated
with a scalar `new`, using `delete[]`. Both fixed.

**`BalbiNov2011` responds non-physically to live fuel moisture at the values
in the shipped fuel table.** `xsi` exceeds 1 for fuel 1 of
`tests/runff/fuels.csv`, the flame temperature term goes negative, and `R00`
raises it to the fourth power. Rate of spread therefore falls with rising live
moisture up to about `Ml = 0.8` and then climbs again: 1.3e-3 m/s at
`Ml = 0.5`, 1.9e-8 at `Ml = 0.8`, 8.2e-5 at `Ml = 1.0` — which is the value the
table ships. `drier live fuel spreads faster, over the physical range` stops
short of that inversion rather than asserting it is correct.

## Adding a test

Models needing an external resource cannot be covered here: `ANNPropagationModel`
and `BMapLoggerForANNTraining` read a `.ffann` network in their constructor and
abort when it is missing. `tests/runANN` covers those.

For anything else, add its name to the list in `test_model_registry.cpp`. If
`fillStandardConditions` knows every property it reads, it can also join
`standardFuelModels()` in `test_propagation_models.cpp` and inherit the
sweeps; otherwise teach `standardValues()` in `model_sandbox.cpp` the missing
properties first.
