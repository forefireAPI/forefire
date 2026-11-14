# Scenario Studio

The tile-based browser application runs ForeFire in a WebAssembly worker. It
supports perimeter/point startup, manual or gridded wind, a scenario timeline,
fuel barriers and persistent water-drop footprints, FFState restart/download,
GeoJSON/KMZ exports, and ensembles of up to 15 sequential members.

## Build and serve

Build the current engine following [../README.md](../README.md), then stage it:

```sh
python3 bindings/wasm/scenario-studio/prepare.py --corsica
python3 -m http.server 8765 --directory local/scenario-studio
```

Open http://localhost:8765/. Staging copies the freshly built
`bin/forefire.mjs` and `bin/forefire.wasm`; neither is committed. `--corsica`
fetches 15 half-degree tiles covering Corsica into the staged site, with a size
and SHA-256 manifest. Omit it and supply your own `tiles/` directory instead.
Tiles outside the demonstration subset are unavailable and nonburnable. The
2° × 2° domain is fixed; tiles inside it load as the front approaches them.

An empty Cesium token uses the Esri imagery fallback. Set `CESIUM_ION_TOKEN`
before staging to enable your own Cesium services; the token belongs only in
the generated site config. Credits remain visible on the map. Cesium and fonts
load externally; this is not an offline application.

Manual wind works on static GitHub Pages. ERA is optional: set
`FOREFIRE_ERA_PROVIDER` to an HTTP range-capable archive URL when staging, with
CORS for the page origin. Annual MP4 archives are not included. The wind overlay
shows timestamped reference 10 m wind, not reduced or member-perturbed wind.
The ocean bitmap bypass is deliberate for the existing damped archive.

`start.php` is an optional PHP 8.1+ POST launcher, accepting `geojson` and a
UTC `timestamp`. GitHub Pages does not execute PHP. Static startup supports the
page's existing browser message/embedded payload flow; Pyroscope integration
requires a compatible receiver. Server/Nginx configuration stays out of this
source tree.

## Numerical presets and uncertainty

Standard uses 24 m / 6 m perimeter/advance settings. Ultrafast uses adaptive
128 m / 32 m preview settings and a coarser sparse arrival grid, protecting small
ignitions. It can cross narrow roads and changes simulated perimeters; compare
members and the reference with identical numerics. Ensemble defaults to the
reference numerics, with Ultrafast selectable. Default ROS multiplier is 1 and
wind adjustment is 0.7. Danger classes and Farsite fuel mappings are editable
experimental presets, not universal land-cover mappings or site calibration.

Ensemble biases follow the wind/dead-moisture subset of Allaire, Filippi &
Mallet (2020), DOI 10.1071/WF19073, Table 2: truncated direction normal σ 30°;
wind factors 2/3–1.5 from a truncated log-normal; dead-moisture factors 0.7–1.3.
Biases stay constant per member. Probability is the fraction of members burned
by the selected time; P10/P50/P90 zones use all members in the denominator.
Initial burned ground is grey. Maps are 384 × 384 raster previews over retained
perimeters, not native arrival maps. The first hour retains solver snapshots,
then hourly snapshots and the final state. Fifteen members give 6.7-point
probability increments. Boundary-clipped members are flagged.

RESET recreates the worker from the initial state and retains scheduled actions.
The map's new-case icon clears the case. An imported perimeter is already burned
at startup in all members. FFState restart retains its original checkpoint clock.

## Checks

From the repository root, after staging:

```sh
node bindings/wasm/smoke.mjs bin/forefire.mjs
node --test local/scenario-studio/tests/*.test.mjs
FFTILE_FIXTURES="$PWD/test_output_tiles" node --test local/scenario-studio/tests/*.test.mjs
```

The first Node suite skips real-tile WASM checks when fixtures are absent; the
second runs them. Tests cover holes, known initial burn, no future perimeter in
the first minute, slower/faster perturbations, reset and separate-worker members.
Browser codec/map rendering requires browser QA. Tests here do not validate
Meso-NH coupling; see [tests/ffstate](../../../tests/ffstate/README.md).

Vendored JS licenses and provenance are in `vendor/`. The compiled engine uses
the repository LICENSE. `assets/fuels-experimental.csv` and its mapping JSON
must be reviewed before interpreting Farsite results scientifically.
