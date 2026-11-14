# FFState checkpoints

FFState stores the live fire objects and their pending motion, arrival history,
effective fire parameters, fuel table, forcing and supported modifiers in one
versioned file. Base FFTile landscapes remain external. Native builds can use
this with NetCDF disabled; the same implementation runs in WebAssembly.

## Commands

```text
saveState[file=current.ffstate]
```

In a fresh session:

```text
loadData[current.ffstate]
step[dt=3600]
```

`loadState[file=current.ffstate]` is equivalent. Relative checkpoint paths use
the existing `caseDirectory/ForeFireDataDirectory` convention. An existing
domain is never implicitly replaced. The simulation resumes at the stored
clock; loading alone does not advance it.

An external scenario applies to the restored state once:

```text
loadState[file=current.ffstate]
include[/path/to/scenario.ff]
step[dt=3600]
saveState[file=scenario-result.ffstate]
```

`saveState[file=current.ffstate;scenario=/path/to/scenario.ff]` can carry a script
as an optional named payload. Loading does not execute it. Ordinary external
`include[]` keeps its existing path/command semantics. Previously applied fuel
table changes and loaded fuel PNG overrides are saved as effective data, rather
than replayed commands. `provider=...` on a standalone `loadState` relocates the
external tile source; known tile hashes must still match.

Finish initialization with `step[dt=0]` or a real step before saving. Save from
the controlling thread between completed steps. In coupling, invoke the command
on the fire master at a completed coupling barrier, using the existing
`executeMNHCommand` entry point if calling from the host. This does not make
concurrent HTTP mutation of a running solver thread safe.

## Attach to Meso-NH

Meso-NH must already provide its geographic registration and MPI domain. In its
`ForeFire/Params.ff`, configure the checkpoint before the domains/models are
constructed:

```text
setParameter[ffstateFile=/absolute/path/current.ffstate]
```

Its `ForeFire/Init.ff` can then be just:

```text
loadState[mode=attach]
```

An explicit `loadState[file=...;mode=attach]` is also available. In the MPI
coupler it creates the master fire domain from the registered host partitions;
the source domain does not replace the Meso-NH domain. `ffstateFile` prepares
saved fire settings before model construction. Atmospheric wind, geometry,
time origin and MPI settings stay host-owned. The host configures its tile
provider; the checkpoint supplies content identities and saved fuel overrides.
Those overrides and the effective fuel table are also restored on the
atmospheric ranks.

A standalone state becomes coupled at the same physical UTC instant. Positions,
pending locations and vectors map through the source geography into the parent
Cartesian frame. Burn ages survive any time-origin offset. When forcing changes
from standalone wind to Meso-NH, future motion is rescheduled while keeping the
current velocity for relaxation; stopped nodes stay stopped. On a matching
coupled grid and numerical setup, saved pending node motion is retained.

The host arrival grid must be at least as fine as the source. `ffstateFile`
chooses a compatible Cartesian `arrivalMapResolution` before host construction;
this is independent of perimeter resolution, spatial advance and minimum front
depth. Explicit standalone-to-host conversion resamples the arrival raster;
same-grid restoration copies values exactly. Coarsening and out-of-domain fire
or burned history are rejected. No cropping is performed silently.

Flux integration begins at attachment on every atmospheric rank. Historical
burn ages still determine ongoing combustion; heat/vapour from the interval
before attachment is not injected into the new coupled run. An FFState is a
fire-component checkpoint: a later coupled restart also needs the corresponding
Meso-NH atmospheric state at that UTC instant.

For a scenario applied after startup on every rank, the existing global script
path can schedule an external include, for example:

```text
# Params.ff
setParameter[GlobalInitFile=ScenarioInit.ff]
```

```text
# ForeFire/ScenarioInit.ff, at the chosen model time
include[/path/to/scenario.ff]@t=54300
```

The test scenario changes effective fuel moisture. Arbitrary scenario commands
must respect the existing coupling synchronization and ownership rules.

## Stored data and boundaries

The `FFSTATE1` header carries a format version and named chunks. Each chunk has
a length and CRC32. Numeric binary fields use fixed-width little-endian values.
There are no raw object layouts, vtables or memory pointers. The manifest records
the engine release; matching model implementation/data remain necessary for
scientific reproducibility across builds.

| Chunk | Data |
| --- | --- |
| `domain` | Exact clocks, UTC reference, source frame/grid and arrival layout |
| `objects.ff` | Full-precision ASCII front/node records with stable identities and references |
| `events` | Pending event times, types and ordered object references |
| `arrival` | Sparse finite arrival values, losslessly run-length encoded as index/count/float64 triples; omitted cells are unburned infinity |
| `parameters` | Effective parameter values and effective fuel table |
| `tiles` | SHA-256 identities of observed base tiles, confirmed missing coverage and compressed fuel PNG overrides |
| `wind` | Optional rolling reference U/V field, grid and timestamps, including missing-value masks |
| `fluxes` | Uniform flux-layer caches, integration clocks and explicit emissions |
| `runtime` | Output/iteration bookkeeping and command reference clock |
| `scenario.ff` | Optional script bytes; never executed just by opening a state |

`objects.ff` uses the `FFOBJECTS1` record schema, not the ordinary executable
print/import syntax. A front record is `id parent time update expanding`. A node
record is `id front merging state time update location[3] next[3] velocity[3]
normal[3] speed depth curvature`. References address records, not pointers;
zero merging reference means none. Only a pending merge requires its partner.
Full-precision ASCII round-trips the doubles; a separate binary node codec is
unnecessary. Normal `print[]` output stays unchanged.

The reader checks checksums, dimensions, references and time consistency before
publishing a standalone session. Saves use a temporary file and rename, leaving
an existing checkpoint intact on validation failure. The file limit is 512 MiB.
An observed missing tile stays missing after restoration, and changed known
tile bytes are rejected when needed. Unvisited future tiles still require an
immutable provider version for reproducibility.

The first implementation supports Rothermel, Balbi 2015, Farsite, Iso and
WindDriven over FFTiles. It does not serialize arbitrary custom raster layers,
spatial flux-model maps, external ANN internals, or pending executable scripts.
Unsupported recognized state produces an error rather than a partial file.
Complete pending scripts before saving, or keep future scenarios external.
Coupled-source files currently restore through `mode=attach`, not into an
unregistered standalone domain. Weather-window renewal remains the caller's
job; the saved window must cover any step until it is renewed. Web file-picker
controls are not included in this core change.

Constructor-read model settings (such as wind-reduction coefficients) must be
changed through model restart, as in the current web controls; arbitrary raw
parameter-string edits do not reconfigure an already constructed model. Live
node numerics and fuel-table changes are captured from their effective values.

Use fresh processes for independent native ensemble members, or fresh WASM
modules. This feature does not remove the core's existing process-global state.

## Checks

Native unit tests include live/pending object restoration, stopped segments,
both arrival layouts, loaded fuel overrides, SHA-256 vectors, corrupted states
and changed-tile rejection. They run in `unit.ffstate` with the ordinary CTest
suite. A fresh-process test compares against an uninterrupted real tile run:

```bash
python3 tests/ffstate/smoke.py --exe local/ffstate/build/forefire \
  --tiles test_output_tiles
node tests/ffstate/wasm.mjs
```

The latter checks native-to-WASM object/event/arrival identity, continued spread,
and rolling-wind restoration in a fresh module. Supply module/case/tile paths as
its three positional arguments when using another build.

Run the actual four-rank nested case with a checkpoint at the input atmospheric
time (2017-06-17 15:00 UTC):

```bash
python3 tests/ffstate/coupled.py --mesonh-root /path/to/meso-nh-6-dev \
  --state /path/to/fire.ffstate --out local/ffstate/coupled-run
```

Add `--scenario tests/ffstate/scenario.ff --duration 600` to apply the example
at the halfway point. This driver uses the existing nested-case inputs and
never replaces them. It requires the updated MPI-enabled ForeFire library
linked as Meso-NH's `exe/libForeFire.so`. No Fortran source change was needed
for the tested build; the changes are in ForeFire's C/Fortran coupling layer.

## Verified on this Mac, 2026-11-13

- Both native arrival layouts resumed in fresh processes with exact node,
  pending-event and arrival records relative to uninterrupted runs, including
  fractional time across a UTC year boundary.
- Native-to-WASM records round-tripped exactly; continued propagation and a
  second WASM instance with rolling wind passed. The check exposed and fixed
  a pre-existing 32-bit `long` shift that created zero node IDs in WASM.
- An evolved standalone state with 81 nodes attached to the registered nested
  Cartesian frame. Coordinate error was below 0.01 m; burn-age extrema were
  preserved. Arrival-raster area changed by about 0.73% during resampling.
- Both atmospheric domains completed 1,200 seconds. Fire area grew from
  25.14 ha to 179.46 ha; all 336 audited exchange rows were finite, with nonzero
  heat/vapour returns and both atmospheric checkpoints written.
- A second run restored the coupled checkpoint with identical live-object and
  arrival records. Areas matched the reference through five minutes. The
  external moisture scenario executed once on each of four ranks, then gave
  87.05 ha at ten minutes versus 100.42 ha in the reference (13.3% smaller).

These are restart/integration checks, not validation of atmospheric initial
conditions or calibration of the spread models. The supplied fires do not cross
a processor boundary; the existing master-front/arrival-distribution design was
retained. Validation reports and working cases are under ignored `local/ffstate/`.
