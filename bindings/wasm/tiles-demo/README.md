# Local .fftile / WebAssembly demo

A local HTTP server supplies the page, WASM module, fuel table and compressed
`.fftile` files. **The simulation runs in a browser worker, not on the
server.** The demo needs no external map service or JavaScript framework.

It opens near 8.9 E, 42.01 N in Corsica, on the converted landscape. The
0.08-degree by 0.04-degree footprint crosses latitude 42, so loading the
initial view fetches both `41.5_8.5.fftile` and `42_8.5.fftile`.
The fuel properties come from `tests/runff/fuels.csv`, which includes the IDs
in these legacy tiles. Choose a matching table before adapting the demo to
another dataset; fuel IDs are not universal model definitions.

## Build and start

From the repository root, with Docker:

```bash
docker run --rm -v "$PWD":/w -w /w emscripten/emsdk:6.0.6 bash -lc '
  emcmake cmake -S . -B local/fftiles/wasm-build -DCMAKE_BUILD_TYPE=Release &&
  cmake --build local/fftiles/wasm-build -j6'
python3 tools/tiles/serve_wasm_demo.py --tiles test_output_tiles --port 8765
```

Or use a locally installed Emscripten SDK with `emcmake cmake` and the same
build directory. The generated `bin/forefire.mjs` and `bin/forefire.wasm`
are ignored by Git, as are `local/` and `test_output_tiles/`.

Open **http://localhost:8765/**. The default service is
`https://forefire.univ-corse.fr/fftiles/`. The local server forwards this
specific service through `/public-tiles/`, because its static host currently
omits browser CORS headers. Conversion and simulation stay separate: no
simulation runs on either server. To use the local converted files instead,
set Service address to `http://localhost:8765/tiles/`.

- Scroll, pinch, or use + / − to zoom. Drag to pan; **Fit domain** resets the
  view. Zooming samples the visible tile data again for a detailed preview,
  rather than enlarging the overview pixels. The solver always uses the
  original tile resolution.
- Click burnable ground to select a location, then **Ignite & run**. During
  a run, select another location and press **Add fire at selected point**.
  Ignitions use the current simulation time.
- Wind speed and direction apply immediately between solver steps. The arrow
  points where the wind blows, rather than its meteorological source.
- **Pause simulation** and **Continue simulation** retain every fire and the
  simulation clock. The duration is an additional interval. Select a slower
  simulation speed for more interaction time. **Reset simulation** clears
  the fires and clock; loading a new landscape also starts a fresh worker.
- The timeline replays recorded frames. New fires and wind changes always
  apply to the live solver time, even when viewing an earlier frame.

Use Service address, Longitude, Latitude and Domain, then **Load landscape**,
to inspect another footprint. The maximum fixed domain is 2° × 2°; the
initial smaller Corsican footprint loads quickly. Zoom and pan stay inside
that loaded simulation domain and do not expand it. Missing tiles and bare
or water fuel rows are nonburnable. Transport and decoding errors are shown.
The scale and footprint use Cartesian meters, with north at the top.

The server binds to loopback and exposes whitelisted assets, local tiles and
a fixed public tile origin. It does not accept arbitrary proxy destinations.

## Browser transport

`../tile-service.mjs` implements the asynchronous browser side of the native
service-and-center contract. It computes the same fixed geographic envelope,
fetches intersecting tile addresses (three requests at a time), validates
headers/addresses/CRC and stages **compressed files** into Emscripten MEMFS.
It then calls:

```text
loadData[provider=/tile-service;center=(longitude,latitude);size=(width,height);prefetch=0]
```

The existing C++ provider reads that compressed cache and the C++ `.fftile`
reader decodes/paginates fields when queried. The browser does not convert
JPEG/PNG into separate array-backed simulation layers. Both JPEG8 and the
PNG16 altitude extension are handled by the same decoder as the native build.

This deliberately uses prefetch for the fixed domain: synchronous C++ field
lookups never block on an asynchronous browser network request. It needs
neither libcurl nor Asyncify. MEMFS lasts for the worker's lifetime; it is not
a persistent offline browser store. HTTP 404/410 files are absent and become
zero coverage. Other transport or corrupt-header errors stop loading.
Full codec validation occurs when the C++ reader loads a tile. Boundaries
may be indicated by dashed half-degree lines.

`ForeFire.tileDomain()` returns footprint/origin/cache metadata.
`ForeFire.sampleLayer(name,nx,ny)` samples a bounded preview (maximum
1024 x 1024), returning `{width,height,data}` with south-first rows.
`ForeFire.sampleWindow(name,x,y,width,height,nx,ny)` samples a bounded
subregion using the same south-first convention.
`ForeFire.sampleAt(name,x,y)` queries the actual data layer at a point.
These avoid materializing a full-domain high-resolution fuel matrix.

## Checks

```bash
node bindings/wasm/smoke.mjs bin/forefire.mjs
node --test bindings/wasm/tile-service.test.mjs bindings/wasm/tiles-demo/interactions.test.mjs
# With the server running and the two default Corsican tiles present:
node bindings/wasm/tiles-demo/smoke.mjs http://127.0.0.1:8765
# Same checks using the public service through the local forwarder:
node bindings/wasm/tiles-demo/smoke.mjs http://127.0.0.1:8765 https://forefire.univ-corse.fr/fftiles/
```

The final check runs the actual demo worker under Node with a worker-message
shim, fetches real tiles over HTTP, samples terrain through the C++ bindings,
and checks detailed preview sampling, a second ignition, wind updates,
pause/resume and ten minutes of Rothermel spread with finite coordinates.
It does not test the browser UI. No Meso-NH coupling is involved.
