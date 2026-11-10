# ForeFire tiles

`fftile.py` writes and inspects self-contained, half-degree `.fftile` files.
The C++ reader in `src/fftile.cpp` uses the vendored stb decoder and needs no
NetCDF, HDF5, PROJ or network library. NetCDF-backed I/O remains optional for
existing cases.

## Write a tile

Install `requirements.txt`, then import `geometry`, `pixel_centers` and
`write_tile` from `fftile.py`. Supply a 2D fuel array and seven continuous
arrays: altitude, slope east/north (`dz/dx`, `dz/dy`), and the four wind matrix
coefficients (`muu`, `muv`, `mvu`, `mvv`). Rows run **south to north**; columns
run west to east. Fuel contains model IDs in 0..255, sampled by nearest
neighbour; zero is nonburnable/no data. Never average categorical fuel IDs.
Fuel pixels use a global angular lattice: 5538 pixels per half-degree core
and 111 halo pixels on each side (5760 pixels across, about 10.04 m
north/south). Its halo is about 0.010022 degrees. Keeping fuel longitude
spacing fixed avoids categorical mismatches when adjacent latitude bands
would otherwise have different widths. This intentionally replaces the
plan's cosine-scaled fuel width and exact 0.01-degree halo; continuous grids
still use cosine-scaled widths. Fuel resolution east/west is finer at higher
latitudes. A tile uses about 33 MB for fuel, rather than 25 MB at 42 degrees.
At a pole the absent halo rows are removed from the fuel image.

The writer rejects nonfinite continuous values and fractional/out-of-range
fuel IDs rather than silently clipping them.

```python
from fftile import geometry, pixel_centers, write_tile

core, bounds, dimensions = geometry(42, 8.5)
# dimensions is ((fuel_w, fuel_h), (alti_w, alti_h),
#                (slope_w, slope_h), (wind_w, wind_h)).
# Resample source fields at pixel_centers(bounds, width, height), then:
report = write_tile("tiles/42_8.5.fftile", 42, 8.5,
                    fuel, altitude, sx, sy, muu, muv, mvu, mvv)
```

No-data regions must have fuel zero. Compute slopes in meters before
compression, excluding coverage edges from terrain derivatives. The generic
writer deliberately does not assume any source dataset, projection or wind
response convention. NetCDF conversion needs Python-side dependencies only;
local conversion scripts and source data can live in ignored `local/`,
`test_input_tiles/` and `test_output_tiles/`.

Inspect the header or extract the original compressed payloads:

```bash
python tools/tiles/fftile.py tiles/42_8.5.fftile --inspect
python tools/tiles/fftile.py tiles/42_8.5.fftile --unpack local/tile-preview
python -m unittest discover -s tools/tiles -v
```

## Run without NetCDF

```bash
cmake -S . -B build -DFOREFIRE_ENABLE_NETCDF=OFF -DFOREFIRE_ENABLE_MPI=OFF
cmake --build build -j
```

The native build automatically enables tile HTTP support when libcurl is
available. Wheels and WASM default to local-only support. Set
`-DFOREFIRE_ENABLE_TILE_HTTP=OFF` for a local-file-only build.
No NetCDF, HDF5 or PROJ is needed for either path.

### Center and service in one command

```text
loadData[provider=https://tiles.example.org/landscapes;center=(8.5,42)]
```

The provider directory serves canonical filenames such as
`https://tiles.example.org/landscapes/41.5_8.5.fftile`. No catalog or directory
listing is required. Center order is **longitude,latitude**.

The service supplies static landscape data. The propagation model, matching
fuel-property table, and weather remain part of the case configuration.

This creates a fixed Cartesian domain centered on that point, within a
2-degree by 2-degree geographic envelope. It immediately prefetches the
intersecting tile addresses, downloading compressed files into
`caseDirectory/ForeFireDataDirectory/tiles-cache/<service-key>/`. Downloads are decoded briefly for validation before they enter the disk
cache, then those validation buffers are discarded. The decoded LRU stays
empty until the simulation queries the landscape. Available cached
files are reusable offline; they are treated as immutable until removed.
The Cartesian rectangle is inscribed in the geographic envelope because its
longitude scale changes with latitude. It does not expand while running.

Optional arguments:

```text
loadData[provider=https://tiles.example.org/landscapes;center=(8.5,42);size=(1,1);date=2026-11-10T12:00:00Z]
loadData[http://localhost:8000/tiles;center=(8.5,42);size=(0.5,0.5);prefetch=0]
loadData[provider=/path/to/local/tiles;center=(8.5,42);size=(0.5,0.5)]
```

`size` is longitude width,latitude height in degrees; each must be positive
and at most 2. `prefetch=0` registers the provider but postpones fetching until
queries need it. `date` sets the reference date, while simulation time starts
at zero. Clear an existing domain before changing provider/center. Invalid
arguments are rejected before creating a domain. The geographic envelope
must not touch or cross a pole. Antimeridian crossings are supported.

HTTP 404 and 410 are missing coverage and yield zero fields/zero spread.
Other HTTP statuses, network failures and corrupt/wrong-address tiles are
errors, so a service outage cannot silently become a firebreak. A prefetch
failure clears the newly created domain; already downloaded cache files
remain for the next attempt. Transfers time out after 30 seconds (10 seconds
to connect), follow at most 5 redirects and limit each response to 64 MiB.
HTTPS certificate verification remains enabled, and an HTTPS source cannot
redirect to plain HTTP. Base service URLs cannot contain query strings or
fragments; authentication/header configuration is not exposed by this command.
Meso-NH coupling and browser-side HTTP loading are separate from this native
standalone entry point.

### Explicit domain configuration

Set `fftilesPath` **before** constructing the domain. The directory resolves
like other ForeFire paths, relative to `caseDirectory/ForeFireDataDirectory`
unless absolute. A separate domain declaration makes the simulated area
independent of tile coverage, including areas for which tiles do not exist.
For example, with a global `tiles/` directory alongside `cases/`:

```text
setParameter[caseDirectory=cases/demo]
setParameter[ForeFireDataDirectory=ForeFire]
setParameter[fftilesPath=../../../tiles]
setParameter[SWLngLat=8.49,41.99]
setParameter[propagationModel=Rothermel]
setParameter[windU=10]
setParameter[windV=10]
FireDomain[sw=(0,0,0);ne=(20000,20000,0);t=0]
startFire[lonlat=(8.495,41.995,0);t=0]
trigger[wind;vel=(5,10,0)]@t=3600
```

The Cartesian SW corner corresponds to `SWLngLat` (longitude, latitude).
Positions, speed and solver geometry stay in meters. Geographic sampling,
ignition and perimeter output share the same latitude-dependent local
transform, including nonzero Cartesian origins and longitude wrapping. This
is an approximation to the Earth, not a projection library; large-domain
physical distortion has not been validated. Raster exports use the bounding
geographic corners of their Cartesian rectangle; they are not a reprojected
geographic raster. Meso-NH coupling and the WASM build have not been validated
by the native tile tests.

## Providers, overrides and missing coverage

`FFTileProvider` is the extension boundary for local, HTTP or application-managed
retrieval. It returns compressed bytes, not decoded matrices. The local
provider tries these paths in order:

1. `caseDirectory/ForeFireDataDirectory/tiles/{lat}_{lon}.fftile`
2. `fftilesPath/{lat}_{lon}.fftile`

`DataBroker::loadFFTiles("")` also accepts `LAYERS_PATH` from simulation
parameters or the environment when no global directory is given. HTTP(S) sources use `FFTileHTTPProvider`; local paths use
`FFTileFileProvider`. HTTP cache namespaces are isolated by service URL.

A matching case-local `{lat}_{lon}_fuel.png` overrides the fuel payload from
either source. It must be 8-bit grayscale, south first, and have exactly the
base tile's fuel dimensions. Reload/clear the cache to pick up file edits;
these are immutable views, not writable `setMatrix`/`trigger[fuelIndice]`
arrays. A fuel override requires a base tile. Exporting a full layer through
`getMatrix` is limited to 16 million samples; direct point/raster queries page
without materializing the whole domain. Legacy binary layer dumps are not
supported by tile views.

Missing files yield zero fields and zero spread locally, even for models
that do not consume fuel. Present but malformed files throw a descriptive
error. Missing tiles are cached, too; applications downloading data later
call `clearCache()` to retry; this also clears the HTTP provider's remembered
404/410 responses. `fftileCacheSize` sets the number of cached tile
entries (default 8), including negative results. Decoded buffers remain byte
arrays, except for optional 16-bit altitude. The cache belongs to a domain;
it does not make the existing process-global ForeFire engine thread safe.

The **core** determines which tile owns a query. Its halo supplies border
interpolation samples. Sampling never reuses an adjacent tile just because
its halo overlaps: that would make categorical values and local overrides
depend on traversal order. Independent lossy fields are compared at common
geographic coordinates in seam tests; the supplied generator uses a shared fuel lattice so adjacent fuel halos
represent the same cell centers. External generators must preserve that
alignment to obtain the same guarantee.

## Binary contract

All numeric header fields are little endian. The header is exactly 256
bytes; see `fftile_implementation_plan.md` for the field offsets. Python's
`HEADER` explicitly packs those fields; C++ reads them by offset rather than
casting a potentially unaligned, host-endian struct. The reader checks the
magic, bounds, flags, CRC when present, ranges, non-overlapping payloads,
image dimensions/channels and memory limits before using them.

- Magic: `FFTILE01`.
- Flag `0x0001`: row zero is south (required).
- Flag `0x0002`: **altitude uses a lossless 16-bit grayscale PNG**, with the
  existing altitude min/max mapped to 0..65535 instead of 0..255. This is an
  explicit extension to the original JPEG-only plan. Unknown flags fail.
- Fuel: lossless 8-bit grayscale PNG.
- Altitude without flag `0x0002`: 8-bit grayscale JPEG.
- Slope: 8-bit grayscale JPEG, east and north components side by side.
- Wind: 8-bit grayscale JPEG, quadrants `muu, muv / mvu, mvv`.
- CRC32 at offset 200 covers bytes 0..199; zero means unchecked.
- Payload offsets are absolute, consecutive, and begin at byte 256.
- Bounds describe **outer pixel edges**. Cell centers are
  `W + (column + .5) * (E-W)/width`, similarly for latitude.
- Polar halo latitude bounds clip to [-90,90]; antimeridian halo longitudes
  remain unwrapped. Tile addresses wrap longitude into [-180,180).

The writer starts at JPEG quality 92 and increases quality when necessary.
It measures RMSE against the supplied uncompressed target grids: slope
<=0.01 m/m, wind <=0.015, altitude <=min(0.005*range, 0.2 m). If altitude
cannot meet that tighter budget in JPEG, it uses the flagged PNG16 option.
If any budget still fails, it rejects the encoding. A constant field uses
min=max and decodes exactly. The 1 MB size target is informational: accuracy
wins over compressed size. Codec validation does not establish accuracy of
the original data, reprojection or source mosaicking.
