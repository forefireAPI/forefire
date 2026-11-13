# `.ffstate` implementation plan

Status: proposed design, revised for live-object serialization and standalone-to-
Meso-NH attachment; no solver implementation in this change.

## 1. Purpose and command interface

One `.ffstate` file stores one fire state at a known simulation time. It contains
the current node/front objects, arrival times, parameters and effective local modifications needed
to continue. Landscape FFTiles remain external and are identified by their
provider, grid and content references. The file is not a run history.
The saved domain describes the source coordinate frame; it does not require
the destination simulation to use that same domain or to remain uncoupled.

Proposed shell commands:

```text
saveState[file=fire.ffstate]
loadState[file=fire.ffstate]
step[dt=3600]
```

For a new firefighting scenario:

```text
loadState[file=fire.ffstate]
include[scenarios/fuelbreak.ff]
step[dt=3600]
saveState[file=fuelbreak-result.ffstate]
```

Add extension/magic dispatch so the familiar command also works:

```text
loadData[fire.ffstate]
```

It delegates to `loadState`, with the same defaults and validation. Existing
NetCDF and tile-provider commands keep their behavior. Keep `save[]` unchanged;
it currently selects arrival-map or landscape output according to arguments.

`loadState` creates a standalone domain when none exists. With an existing
domain it fails unless `mode=attach` is explicit. It must never silently clear
a running case or create a second domain inside a coupled session.

```text
# After Meso-NH has registered its domain and geographical mapping:
loadState[file=fire.ffstate;mode=attach]
include[scenarios/fuelbreak.ff]
```

Standalone loading restores the saved time and pauses. Attach loads the fire
at the matching Meso-NH UTC instant, using the host's domain and time origin.
It does not advance the simulation or automatically execute a scenario.
Native, Python and WASM use the same core
reader/writer; the browser adds file import/download controls.

## 2. What already exists, and what must change

Relevant code inspected in this checkout:

- `Command::loadData`, `loadTileService`, `include`, `saveSimulation` and `print`
  provide the command integration points in `src/Command.cpp`.
- `StringRepresentation` traverses domain/front/node objects and writes the
  familiar `FireDomain[...]`, `FireFront[...]`, `FireNode[...]` text.
- `FireDomain::saveArrivalTimeNC` / `loadArrivalTimeNC` provide existing arrival
  semantics, but depend on NetCDF, flatten the map, and do not capture a full
  restart clock. The new path must work with NetCDF disabled.
- Arrival storage now has two layouts: legacy Cartesian cell maps and the
  standalone `GeographicArrivalMap` with sparse 0.1-degree blocks.
- `SimulationParameters` exposes `getAllKeys()` and typed/string accessors.
- `CLibForeFire.cpp` constructs Meso-NH domains after registering the host grid;
  it owns the coupling lifecycle.

Ordinary print/reload is not an exact checkpoint today:

- Node/front/domain string output does not consistently use full double
  precision.
- `Command::addFireNode` maps a saved `moving` state to `init`, can interpolate
  extra nodes, and normal front completion performs initialization work.
- Print omits pending node motion, scheduler details and potentially stateful
  model data. Node `nextloc`, atom update times and event times need an audit.
- The event queue uses pointer addresses to break equal-time ties. Recreated
  objects cannot promise identical event ordering across restarts.

Serialize the live object graph through explicit node/front/atom snapshot
records. Preserve current and pending values, rather than regenerating nodes
from a geometrical outline. Replace pointers with stable record IDs, then
reconnect those references during restoration. This is field serialization,
not a dump of C++ memory or executable object layouts.

ASCII is sufficient for the node records: use round-trippable double precision
(`max_digits10`), an explicit schema version and fields for all required pending
state. Reuse the visitor, syntax and parsing utilities with a dedicated restore
mode; normal `print[]` can keep its existing concise output. The checkpoint
variant of `FireNode[...]` must include audited fields that ordinary print omits.
Persist front hierarchy, node order, active/final state, current location,
velocity, normal/depth/curvature where authoritative, pending location and atom
update times, plus stable references for relationships required at a save boundary.
Derived values can be omitted only after proving they are safely reconstructible.

No binary node encoding is required in v1. Compressed ASCII node records and
binary arrival blocks coexist inside the binary container. A binary encoding
could later implement the same record schema if size/parse benchmarks justify
it; it must not become a second, inconsistent definition of node state.

## 3. File organization

A versioned binary chunk container, analogous in spirit to FFTile, containing
multiple named logical files. Proposed magic: `FFSTATE1` (8 bytes).

Use a fixed header plus a directory. Each entry records type/name, flags,
64-bit offset, stored and decoded sizes, codec and checksum. Specify byte order,
integer widths and IEEE-754 representation explicitly; never write C++ structs
or pointers directly. Major versions define incompatible layouts; optional
chunks may be skipped, unknown required chunks cause a clear error.

Logical contents:

| Entry | Contents |
| --- | --- |
| `manifest.json` | Format/engine versions, clock, domain/frame, required capabilities, references and chunk inventory |
| `parameters.json` | Effective physics, numerics and layer/model configuration |
| `objects.ff` | Full-precision live front/node/atom records, including hierarchy, identities, states, current and pending motion, and update times |
| `events.json` | Pending event records referring to stable object IDs, with event times/types and external actions where supported |
| `arrival/index` + blocks | Grid description and losslessly encoded allocated arrival blocks |
| `fuels.csv` | Effective fuel property table, including uploaded/custom Farsite properties |
| `modifiers/*` | Current fuel/layer overrides and any stateful model payloads |
| `forcing/*` | Manual wind or currently available rolling forcing window, plus renewal description |
| `scenario.ff` | Optional user-supplied future scenario, stored separately and never run by default |

All numerical state is lossless. Arrival values remain float64 seconds, with
positive infinity for unburned cells; preserve negative historical arrivals.
Do not use JPEG, quantized PNG or a finite unburned sentinel for restart data.
Specify traversal order explicitly, independent of the in-memory layout.

Start with raw and a lossless block codec supported in native and WASM builds;
select a small bundled DEFLATE implementation during implementation rather than
adding a NetCDF/HDF5 dependency. A reader must support raw blocks as the baseline.
Checksums detect corruption; tile content identifiers must use a strong digest
or an immutable dataset revision, not CRC as proof of dataset identity.

Write a sibling temporary file, finish directory/checksums, flush and atomically
rename. A failed save leaves the previous checkpoint intact. Validate lengths,
offsets, duplicate entries, decoded-size limits and required chunks before use.
No extraction to arbitrary filesystem paths is needed.

## 4. Clock and domain contract

Store the exact reference UTC instant, elapsed simulation seconds and checkpoint
UTC instant. Also preserve the engine's `refYear`, `refDay`, `refTime`, command
clock and domain/atom clock conventions and validate their agreement.
Keep fractional seconds; do not reset time to zero on reload. This is necessary
for wind interpolation, arrival ages and heat/vapour fluxes after restart.

The domain record contains Cartesian extents, units, coordinate transform and
geographic coverage. For coupled domains it additionally identifies the parent
frame, nested-domain ID, mass-point grid, rotation and decomposition signature.
Large geography arrays may live in binary chunks referenced by the manifest.

Two restoration paths share the same serialized objects:

- **Standalone restore:** recreate the source domain and its native arrival
  layout, preserving saved current/pending state and numerical settings.
- **Attach:** use the destination domain already provided by Meso-NH, even if
  the state came from an uncoupled run with different extents, origin, rotation,
  arrival grid or processor decomposition. Source geometry is transformation
  metadata, not a replacement for the host geometry.

Standalone-to-coupled conversion is a required feature, not a future optional
remapping tool. Attach performs an explicit, validated transfer:

1. Match checkpoint UTC to the host coupling instant. If time origins differ,
   translate all saved node/event/arrival timestamps by the same offset;
   preserve ages and event delays. Reject a different physical instant rather
   than silently moving the fire forward/backward in time.
2. Map source Cartesian positions through source geography into the registered
   Meso-NH parent Cartesian frame. Apply this to current and pending locations,
   front geometry and modifier footprints. Transform vectors using the local
   differential of the coordinate mapping; account for rotation and metric.
   Restore or recompute derived normals/curvature consistently in the host frame.
3. Transfer arrival history into the supported host arrival layout, retaining
   burn age and burned/unburned meaning. Report when grid conversion resamples
   data. Do not claim a changed grid is a lossless or bitwise-identical restart.
4. Assign physical nodes to host subdomains/ranks using host ownership rules,
   reconnect fronts and create coupling links/halos through existing mechanisms.
   File identities remain logical identities, not source rank ownership.

Require complete coverage of active fronts and the burning-history support
needed for host fluxes. Reject out-of-domain active fire or ambiguous source
geography by default; do not silently clip nodes or infer a projection. A larger
host domain is allowed. Manifest coverage distinguishes known unburned areas
from missing history. Parent/child nesting must not duplicate physical sources.

## 5. Arrival-map storage

Add a small arrival snapshot interface independent of NetCDF:

- Describe the coordinate/grid layout and enumerate allocated blocks.
- Export/import each block directly, without allocating a dense domain map.
- Preserve exact index origins, dimensions, resolution and times.
- Restore into empty arrival storage; do not call initialization scans that
  overwrite saved history. Rebuild derived caches after the authoritative map.

For geographic maps, save subdivisions, absolute global block indices and
allocated blocks. Default-valued cells/blocks can be omitted losslessly.
For Cartesian maps, save FDCell/subcell geometry and allocated data. The format
supports both layouts; geographic maps currently remain standalone-only at
runtime. Attach must nevertheless accept a file containing geographic blocks
and transfer it into Meso-NH's supported Cartesian arrival storage. It does not
require changing the coupled solver to run on geographic blocks.

Same-grid restore copies values exactly. For cross-grid attachment, implement
and test an explicit history remapper. Keep source burn support and ages until
host flux queries are initialized. A point sample can miss a narrow burned
strip; assigning the earliest overlapping time to an entire larger host cell
can invent burned area and heat. Specify the transfer rule and area/age error
budget, with area-aware burn-history aggregation for fluxes where needed.
If the current flux interface cannot represent that history accurately enough,
extend its adapter or reject the unsupported resolution combination explicitly.
Never rebuild historical arrivals from the current front alone.

An arrival map records burned history. Active/passive boundary status comes
from the saved front/node state as well; the arrival map alone does not define
which perimeter segments should remain active.

## 6. External tiles, parameters and modifiers

Save provider URL or local dataset reference, tiling/version information and
known tile identities, including confirmed missing tiles that affected the run.
Do not copy base FFTile payloads. On load, register the provider and fetch only
what current fronts and the normal guard require. Restore known missing-tile
semantics so a newly appeared tile does not silently change the saved case.

Reproducibility requires an immutable provider revision or retained matching
tiles. If known content has changed, stop with an actionable mismatch message;
relocation is explicit (`provider=...`) and does not waive content validation.
Record when provider identity cannot be pinned; do not promise identical runs
against mutable future tiles that were never observed at save time.

Store effective model/numerical parameters, not just changes from current
defaults: ROS, wind adjustment, slope settings where supported, moisture,
relaxation, numerical schemes, resolutions, model choices, and flux configuration.
Include effective adaptive numerics where the runtime differs from the UI target.

Separate simulation parameters from environment settings: cache locations,
output paths, HTTP credentials and process/MPI handles are not blindly restored.
An application chooses new output paths. Attach mode reserves host-owned geometry,
clock, MPI and atmospheric settings. Saved fire physics, fuels, numerics and
modifiers are the baseline; the coupling adapter sets the required coupled
run mode and atmospheric layer bindings. It reports effective changes and
unsupported settings instead of treating `source=standalone` as an error.

Already-applied fuel breaks and suppression modifications must survive. Save
their effective overlay/state, not the commands that historically produced it.
Any runtime-mutated fuel table must be saved from the effective table, rather
than assuming the original `fuelsTable` parameter still matches it.
An unmodified base tile remains an external dependency; its local overrides do
not. Unsupported modified layers or stateful models must fail saving clearly
rather than producing an incomplete checkpoint.

Store future scenario scripts independently. On-demand execution uses the
existing `include[]` mechanism, after restoring the baseline, with a documented
relative-path base. It must not replay past actions or apply a fuel break twice.
If an embedded script is requested later, add explicit `runScenario[]`; do not
execute embedded arbitrary commands merely because a state was opened.

## 7. Forcing and coupled restart

Standalone states preserve manual wind, or the available coarse U/V grid and
timestamps plus the forcing renewal reference. ERA references include variable,
archive/version and sampling settings; the application reopens/renews them at
the saved clock. A core load reports when forcing needs renewal and cannot step
past available coverage. It does not assume a browser decoder exists in C++.

Meso-NH remains the authority for atmospheric state and wind on attach. A saved
standalone forcing layer must not override live atmospheric arrays. An FFState
is a fire-component checkpoint, not a complete atmospheric checkpoint.

The same file can therefore start standalone, then become coupled. Preserve
current fire positions, active states, velocities and historical burn ages;
switch future forcing to Meso-NH. Continuing with different wind is a new
forcing phase, not identical continuation of the standalone trajectory.
Audit pending motions: a future displacement computed with the old standalone
wind cannot blindly remain authoritative after the forcing switch. Refresh
future updates through the engine's normal forcing-change mechanism, retaining
current velocity for relaxation and without relighting or advancing the fire.
If that mechanism is insufficient, define a narrow explicit rescheduling hook
and test it against a live wind change at the same instant.

Coupled integration sequence:

1. Meso-NH starts or restores its atmospheric state at the fire checkpoint UTC
   and registers all nested grids and coordinate mappings.
2. At a coupling barrier, transform and distribute the fire payload into those
   domains, restoring arrivals, live node objects, effective modifiers and
   applicable flux-model memory. An uncoupled source needs no source MPI layout.
3. Register the restored objects with the scheduler, exchange halos and refresh
   host wind/altitude inputs before stepping the fire.
4. Continue at the next coupling interval; avoid duplicate flux injection for
   an already completed interval. Capture any necessary integration counters or
   interval accumulators as part of the coupled payload.
   For a standalone source, initialize host flux accounting at the attachment
   instant: retain ongoing combustion from saved burn ages, but do not inject
   heat for the historical interval before attachment.

Same-grid coupled restart may use a direct restore fast path. Different source
geometry or decomposition uses the attachment transfer above. A
multi-domain FFState can contain namespaced domain/rank chunks with one manifest;
the canonical fire payload must be transferable independently of source ranks.
All ranks save at the same coupling barrier; authoritative owned data exclude
duplicated halo copies. Rank-local staging and a collective success check precede
streamed assembly/publication of the final file. A partial rank set is invalid.
Keep I/O separate from domain attachment so MPI transport can be implemented
without changing the on-disk representation.

Scenarios can be applied immediately after attachment with `include[]`, or
queued while Meso-NH is running and applied at a coupling barrier on all affected
ranks. Fuel/modifier changes must invalidate the relevant caches and pending
propagation updates consistently. Replacing an already running fire state is
a separate explicit replacement operation, not the default meaning of attach.

## 8. Save and restore order

Save only between completed events, or at a completed coupling exchange, while
the simulation is paused. Nodes can retain different last-update times; do not
advance them all to the checkpoint time just to simplify serialization.

Load sequence:

1. Parse and validate the container, clock, layouts, model capabilities and
   available dependencies without changing the active session.
2. Resolve the destination domain and parameter ownership; prepare an isolated
   restoration context. Current singleton/global state needs explicit rollback
   or isolation so a failed load cannot corrupt an existing coupled session.
3. Apply initialization-sensitive fire parameters before creating layers/models;
   in attach mode, retain host-owned settings and prepare the coordinate/time map.
4. Register external tiles; restore effective fuel table and local modifiers.
5. Deserialize the live objects with a dedicated record parser: full precision,
   nesting, identities and current/pending state; reconnect ID references in a
   second pass. No auto-densification and no `moving`-to-`init` conversion.
6. Restore arrivals, numerical continuation fields and model memory without
   rerunning historical burn scans or recomputing saved velocities.
7. Restore clocks and rebuild the scheduler from stable object IDs and saved
   event records; retain required pending external actions or explicitly report
   unsupported ones. Raw pointer addresses are never serialized.
   Attach applies coordinate/time transfer, host ownership and forcing-switch
   rescheduling before publishing the fire to the coupled scheduler.
8. Validate geometry/history/forcing consistency, rebuild spatial caches and
   publish the restored session only when it is usable.

The format supports numerical continuation, but bitwise equivalence is not a
v1 promise: equal-time pointer tie-breaking and cross-build floating-point
differences need separate work. Define measured restart tolerances with tests.
Do not change the event scheduling policy or relaxation physics incidentally.
Label approximate geometry-only imports separately from checkpoint restores.

## 9. Implementation stages and files

1. **Restart audit and contract:** inventory persistent fields for nodes,
   fronts, atoms/events, layers and flux models. Establish uninterrupted-run
   reference cases before choosing which fields are derived. Reject unsupported
   continuation types until their state can be saved.
2. **Container and arrival snapshots:** new `src/ffstate.h/.cpp` and focused
   snapshot interfaces for `GeographicArrivalMap`, `BurningMap`/`FDCell` and
   `FireDomain`. Add lossless block round-trip and corruption tests.
3. **Live-object and parameter restoration:** define one snapshot-record schema
   for nodes/fronts/atoms and ID references; write/read it as full-precision ASCII
   through checkpoint visitors/parser helpers. Include pending fields rather
   than a second partial node definition in a binary sidecar. Preserve the
   current human-readable print command and legacy scripts.
4. **Commands and standalone continuation:** add `saveState`, `loadState`,
   `loadData` dispatch, fuel/overlay and forcing payload support. Wire CMake
   for NetCDF-free native and WASM builds. Python can use shell commands first.
5. **Web controls:** Save state / Open state, imported clock and provider,
   restored manual/ERA settings, original Pyroscope source metadata. Keep file
   serialization in the core; JS handles file bytes and weather renewal.
6. **Meso-NH attach and coordinated checkpoint:** integrate through
   `CLibForeFire`; implement standalone-to-host coordinate/time transformation,
   arrival-history transfer, rank distribution and the forcing transition.
   Preserve host geometry, support coordinated domain/rank payloads and scenario
   changes at barriers, and verify heat/vapour behavior on the real nested case.
   This is a required feature, not deferred as an incompatible-grid import.

## 10. Acceptance tests

- Exact arrival-map round trip for both layouts, including sparse empty blocks,
  negative times, unburned infinity, tile borders and geographic index origins.
- Live-object round trip preserving full precision, nested holes/islands,
  disconnected fronts, node identity, current/pending motion, update times,
  references and states, without added nodes or forced reinitialization.
- Save at T, resume in a fresh process to T+H, and compare against an uninterrupted
  run using identical forcing/tiles/numerics. Measure geometry, area, arrivals,
  node states and event times, not just a plotted outline. Include relaxed road
  crossing, merging/splitting, stopped segments and a newly added ignition.
- Effective fuel edits and custom tables survive without replaying old scripts;
  a new external scenario applies exactly once.
- Fractional clocks, historical burns and UTC year transitions preserve wind
  interpolation and burning age. Missing future forcing stops cleanly.
- Truncated/corrupted/unsupported files and invalid time/geography/provider mappings fail without
  destroying a usable session; interrupted saves retain the previous file.
- Native-to-WASM and WASM-to-native restore with NetCDF disabled, including
  dynamic tile loading and ERA renewal requests.
- Nested MPI restart at a coupling boundary: correct parent Cartesian placement,
  no duplicate fronts/halos or repeated heat/vapour injection, and flux/time-series
  agreement with the uninterrupted coupled run within declared tolerances.
- Standalone geographic-grid state attaches to a rotated/offset nested Meso-NH
  Cartesian domain at the same UTC with a different reference epoch. Verify
  transformed current/pending positions and vectors, unchanged burn ages,
  active/passive status, arrival-transfer error and rank-boundary ownership.
- Standalone-to-coupled forcing switches to host wind without a duplicate step,
  stale old-wind displacement or relighting; ongoing heat flux uses saved ages.
  Apply a fuel-break scenario at a later coupling barrier and verify all ranks
  use it once. Test an active front crossing a processor boundary.

First deliverable: standalone single-file checkpoint with external FFTiles,
lossless arrivals, serialized live objects, effective parameters/modifiers and a
tested continuation path. The full requested feature also includes transfer of
that standalone state into a Meso-NH-owned domain and later scenario application;
it is not declared complete by standalone round-trip tests alone.
