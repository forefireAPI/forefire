# ForeFire in WebAssembly

Follow-up to [issue #171](https://github.com/forefireAPI/forefire/issues/171):
compile the engine to WebAssembly and run a simulation with no NetCDF, in Node
or in a browser tab.

This is an experimental build, not a supported one yet. It answers the three
questions the issue left open: does it link, does it run, how big is it.

`.github/workflows/wasm.yml` builds the module and runs `smoke.mjs` on every
pull request, and `.github/workflows/wasm-pages.yml` publishes the browser
demo to GitHub Pages on every push to `dev`. The `.wasm` is built in CI rather
than committed: it is a megabyte of generated output, and a stale binary
sitting next to live sources is worse than a two minute build.

## What made it possible

`#include <netcdf>` sat in `src/DataBroker.h`, and the `using namespace netCDF`
next to it pulled the dependency into 48 of the 58 files in `src/`.
`-DFOREFIRE_ENABLE_NETCDF=OFF` (#208) compiles that include out, along with
every code path behind it, and leaves a stub in each entry point that reports
the missing support. The command table is unchanged, so a script that reaches
for NetCDF gets a message instead of a link error.

Nothing else needed touching. MPI was already behind `MPI_COUPLING`, the HTTP
server and `popen` are never reached from the WASM entry point, and `termios`
lives only in the CLI shell, which a WASM build does not compile.

## Building

Needs Emscripten. With the SDK image:

```sh
podman run --rm -v "$PWD":/w:z -w /w docker.io/emscripten/emsdk:6.0.6 bash -lc '
  emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=Release &&
  cmake --build build-wasm -j8'
```

`emcmake` sets `EMSCRIPTEN`, which turns off NetCDF, MPI, the Python module,
the unit tests and the CLI shell, and turns on `forefire_wasm`. The outputs
land in `bin/`:

| File | Size | gzip | brotli |
| --- | --- | --- | --- |
| `forefire.wasm` | 990 KB | 298 KB | 224 KB |
| `forefire.mjs` | 131 KB | 33 KB | — |

Roughly 257 KB over the wire with brotli, for the whole engine and all its
propagation and flux models.

## Running

Headless, under Node:

```sh
node bindings/wasm/smoke.mjs bin/forefire.mjs
```

In a browser — copy the four page files next to the build outputs and serve
the directory, since a `file://` page can neither fetch the `.wasm` nor start a
module worker:

```sh
cp bindings/wasm/{index.html,worker.mjs,landscape.mjs,terrain3d.mjs} bin/
python3 -m http.server -d bin
```

The solver runs in a worker (`worker.mjs`), which computes the **whole run up
front** — a few hundred milliseconds for a modest fire, a few seconds for one
that burns 800 ha — and the page then scrubs the result on a timeline. Off the
main thread is what keeps the page answering while that happens.

Each run gets a fresh worker. The engine's state is process-wide (#175), so a
second `FireDomain` in the same module keeps the first one's propagation model,
and switching models between runs would silently do nothing.

### Explore and Research

The page has two modes over the same map:

- **Explore**, the default: click the map to light a fire, pick a wind on the
  dial or from a few scenarios, and watch it play back.
- **Research** adds the propagation model, `propagationSpeedAdjustmentFactor`
  and `windReductionFactor`, the time step, step count, `perimeterResolution`
  and `spatialIncrement`, dead and live fuel moisture and bed depths (the `Md`,
  `Ml` and `e` columns of the fuel table), and the excess-green cut-offs
  between fuel classes. Each control shows the ForeFire name it sets.

Two layers read the run itself. **Arrival** colours the burnt ground by when
it caught fire, with labelled isochrones at a round interval: the arrival-time
map ForeFire writes natively, rebuilt from the perimeter of every frame.
**Spread** colours the front by each node's rate of spread, the length of the
`vel` that `print[]` reports, scaled to the run's 95th percentile so a few fast
nodes on a tight curve do not wash it out.

The models on offer are the ones that run on this fuel table with only slope
and wind: Rothermel, BalbiNov2011Curv, BalbiUnsteady, TroisPourcent and Iso.
Balbi2015, Balbi2020 and BalbiNov2011TMdMl also need moisture and temperature
layers and abort without them; BalbiNov2011 places the front far from the
ignition.

### The landscape comes from map tiles

`landscape.mjs` fetches two tiled layers for a square of ground and decodes
both into layer arrays. No NetCDF, no GIS stack: tiles in, an `Int32Array` and
a `Float64Array` out, straight into `addIndexLayer` and `addScalarLayer`.

The page opens on a 4 km square of the Ajaccio hinterland — the site of
`tests/runff/run.ff` — but the map is live: drag to pan, scroll to resize the
domain between 500 m and 30 km, and click to light a fire. The tile zoom
is chosen from the domain size so the mosaic stays around a thousand pixels and
36 tiles whatever the scale. Panning and zooming keep the run playing: each
run remembers the square it was computed on, and its fronts are drawn wherever
that square now sits on screen. The fetch is debounced, and the canvas previews
the move with the tiles it already has.

The ignition point is stored as a longitude and latitude rather than as domain
metres, so panning and zooming leave it on the same patch of ground instead of
sliding it across the map.

**Fuel**, from satellite imagery, classified by excess green `g - (r+b)/2`
into three rows of the repository's own `tests/runff/fuels.csv`:

| Class | Cut | `fuels.csv` row | Share of the site |
| --- | --- | --- | --- |
| shrub / maquis | ≥ 20 | index 5, bed depth 0.6 m | ~44% |
| sparse / grass | ≥ 10 | index 4, bed depth 0.19 m | ~32% |
| bare, built, water | below | index 0, no fuel bed | ~24% |

**Elevation**, from Terrarium-encoded SRTM, `r*256 + g + b/256 - 32768` metres.
The site runs from 11 m to 761 m.

Both are load-bearing, because the default model is **Rothermel** with the
parameters from `tests/runff/params.ff`, and its rate of spread reads the fuel bed and the
slope. Index 0 has no fuel bed, so the fire stops at clearings, tracks and
rooftops. And on a bare slope with no wind at all, ten minutes of spread grows
with the grade while the downhill side stays put:

| slope | uphill | downhill |
| --- | --- | --- |
| flat | 47 m | 45 m |
| 30% | 99 m | 45 m |
| 60% | 210 m | 43 m |

> **Layer order matters.** The `altitude` layer must be registered *before* the
> propagation layer. `DataBroker` derives the slope layer at the moment a layer
> named `altitude` is registered; a model that asks for `slope` first gets a
> constant zero-altitude stand-in, and the terrain silently does nothing.

If the tile hosts are unreachable the page falls back to uniform fuel on flat
ground and says so.

### Knowing when the fire stops

A fire that stops does not leave a stationary perimeter behind: ForeFire
discards the front and `print[]` returns nothing at all, which on a canvas is
indistinguishable from a bug. The worker detects the front list emptying, drops
that frame, and reports why it ended — `left-domain` if the last live front was
against the boundary, `burnt-out` otherwise. A slower death, where the
perimeter survives but stops advancing, is caught by a burnt-area plateau over
a twelve-step window (`stalled`), and an ignition on bare ground is reported as
`never-caught` rather than as a fire that stopped.

### 3D terrain

`terrain3d.mjs` draws the elevation field as a textured mesh in raw WebGL — no
library, since a heightfield with one directional light is not worth a megabyte
of dependency. The texture is whatever the 2D view painted, so the two views
cannot disagree. Drag to orbit, scroll to zoom the ground in or out (which
fetches new terrain, as in 2D), and the vertical exaggeration is adjustable
because 750 m of relief over 4 km is subtle at true scale. The view degrades to
2D if WebGL is unavailable.

Switching between the two views is a **camera move, not a swap**. `blendCamera`
interpolates between the orbit and a flat pose — straight down through a
near-orthographic lens (a 0.12 rad field of view from far enough back), framed
so the mesh's unit square exactly fills a square viewport, north up, with the
vertical exaggeration wound to zero. That pose draws what the plain 2D canvas
draws, to within 0.03% of a half-width, so the GL canvas can take over while
still flat and then tilt up: no cross-fade, no second renderer, and no visible
handover. Going back holds the GL canvas until the camera is flat again.

The flat pose stops a third of a degree short of vertical on purpose — straight
down puts the view direction on the up vector and `lookAt` divides by zero.

Clicking the terrain lights a fire there, which needs a ray cast rather
than a coordinate transform: `rayFor` rebuilds the world-space ray through a
pixel from the same camera basis `lookAt` uses, and `raycastHeightfield`
marches it until it drops through the surface, then bisects onto the crossing.
It stops at the first ground it meets, so a click cannot land on a slope hidden
behind a ridge. Picking samples the mesh's own vertex heights, not the
full-resolution field, or a click could land somewhere the visible surface is
not.

`mvpFor`, `blendCamera`, `rayFor`, `raycastHeightfield` and `projectPoint` are
exported so the camera can be checked without a GL context — feed the mesh's corners through and confirm they land in front of
the camera and inside the clip volume. That test exists because the first
version of `multiply` indexed its matrices row-major while `perspective`,
`lookAt` and `uniformMatrix4fv` were all column-major. It still returned a
matrix, and the mesh still drew; it just drew as streaks radiating from a
vanishing point.

## The API

`forefire.mjs` is an ES module exporting a `createForeFire()` factory. It
mirrors the pybind11 module in `bindings/python`, with typed arrays in place of
NumPy arrays — see
[Running from NumPy arrays](../../docs/source/user_guide/python_arrays.rst) for
the concepts, which carry over unchanged.

```js
import createForeFire from "./forefire.mjs";

const Module = await createForeFire();
const ff = new Module.ForeFire();

ff.setString("propagationModel", "WindDriven");
ff.execute("FireDomain[sw=(0,0,0);ne=(4000,4000,0);t=0]");
ff.addLayer("propagation", "WindDriven", "propagationModel");
ff.addIndexLayer("table", "fuel", 0, 0, 0, 4000, 4000, 0, 200, 200, 1, 1, fuelArray);
ff.execute("startFire[loc=(2000,2000,0.);t=0]");
ff.execute("step[dt=30]");
const front = ff.execute("print[]");
```

The layer methods take the four dimensions explicitly — `nx, ny, nz, nt` — where
the NumPy binding reads them off the array's shape. Otherwise the contract is
identical: pass a flat C-order `(t, z, y, x)` array, indexed
`x + nx * (y + ny * (z + nz * t))`, exactly as `numpy.ndarray.ravel()` would
give you. The binding transposes it into the x-major order the data layers
index with, the same shuffle `_pyforefire.cpp` performs.

## What was measured

- **All 58 files in `src/` compile** under `em++` with
  `FOREFIRE_ENABLE_NETCDF=OFF`. Zero errors. The only diagnostics are seven
  `-Wvla-cxx-extension` warnings, the latent portability issue #171 already
  flags.
- **It links and runs.** `smoke.mjs` runs a full 240 s simulation.
- **The answers match the Python binding.** `smoke.mjs` is a line-for-line port
  of the worked example in the NumPy documentation. Run against a native
  `pyforefire` built from the same tree, the two agree exactly: 349 front nodes,
  x 1980..3905, y 1763..2308, the front stretched downwind as the doc describes.
- **Speed.** The turning-wind demo runs 15 steps of 20 s over a 4 km domain in
  about 1.2 s under Node.

## Known limits

- **Single-threaded.** Emscripten pthreads need `SharedArrayBuffer` and
  COOP/COEP headers, which is exactly the deployment complexity a static page
  is meant to avoid. Nothing in the engine's simulation path threads today —
  only the HTTP server does, and it is not reachable here.
- **The embind translation unit is C++17**; the core stays C++11. embind
  refuses to compile below C++17, and mixing is fine against one libc++.
- **No file I/O worth the name.** `FS` is exported so a caller can stage
  `fuels.csv` or read back what `print[<file>]` writes, but there is no landscape
  file support at all — arrays are the only way in.
