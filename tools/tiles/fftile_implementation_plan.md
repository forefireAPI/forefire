# `.fftile` Global Dynamic Tile Format & Implementation Plan

## 1. Executive Summary & Architectural Goals

This document specifies the design, binary format, conversion pipeline, C++ ForeFire reader architecture, and verification suite for **`.fftile`**—a compact, zero-dependency, dynamically loadable geospatial tile format replacing legacy NetCDF (`data.nc`) files in ForeFire / FireCaster.

### Core Objectives
1. **Eliminate NetCDF / HDF5 / PROJ Dependencies in C++:**
   * Replace `data.nc` (~2.2 MB per tile) with a single self-contained `.fftile` binary file (~300–700 KB per tile) composed of a **fixed 256-byte binary header** followed by **4 standard compressed image payloads** (`PNG` for categorical fuel cover; grayscale `JPEG` atlases for altitude, slope coefficients, and the 2×2 wind Jacobian tensor).
   * Decode in C++ using only standard `fread` + single-header `stb_image.h` (`stbi_load_from_memory`).
   * Decode in Web Browsers / JavaScript directly via `ArrayBuffer` + `DataView` + `Blob` / `createImageBitmap()`.
2. **Global `0.5° × 0.5°` Grid with Built-in Border Halo:**
   * Tiles are indexed globally on a regular **`0.5° × 0.5°` latitude/longitude grid** (`LAT_LON`), aligning 1-to-1 with **ERA5 (`0.25°` = 2×2 cells per tile)** and **ERA5-Land (`0.1°` = 5×5 cells per tile)**.
   * Each tile includes a **`±0.01°` border halo** (`0.52° × 0.52°` total encoded extent, ~1.1 km margin on all 4 sides) so bilinear interpolation at tile boundaries never needs to stitch pixels across two tiles at runtime, and fire fronts oscillating near a boundary do not thrash-load neighboring tiles.
3. **100% Cartesian Meter Space in the ForeFire Solver (Up to 300–500 km Domains):**
   * The ForeFire simulation engine continues to operate **exclusively in Cartesian meters `(x, y)`** (positions, velocities, ROS in `m/s`, normals, curvature, perimeter resolution, and atmospheric coupling).
   * Coordinate conversion between simulation meters `(x, y)` and tile lookup `(lat, lon)` happens **only inside the data layer's `getValueAt(x, y)`** method via a fast 2-line cosine-corrected local meter transform centered on the simulation origin $(\text{lat}_0, \text{lon}_0)$.
4. **Rescaling & Reprojecting from Existing Overlapping `data.nc` Tiles:**
   * Existing precomputed `data.nc` tiles (spaced on a `0.25°` grid, each covering ~`64 km × 64 km` $\approx 0.57^\circ \times 0.77^\circ$ with >50% overlap) can be mosaicked, rescaled, and reprojected into clean `0.5° × 0.5°` `.fftile` files without re-running Meso-NH `PGD` or CFD wind simulations.
   * Includes a dedicated **9-overlapping-tile (`3 × 3`) mosaic & verification test plan**.
5. **Dynamic Paging & Local Copy-on-Write Overrides:**
   * ForeFire dynamically loads `.fftile` files on demand as the fire spreads across tile boundaries, checking a simulation-local override directory first so fuel breaks or landcover edits can be applied locally.

---

## 2. Reference Analysis of Existing `data.nc` Tiles

The converter must support existing precomputed tiles (both Generation-2 `WSENLBRT` tiles and Generation-1 `EPSG` UTM tiles).

### 2.1 Existing Folder Naming & Overlap Geometry
* **Folder Convention:** `{lat}_{lon}` snapped to a **`0.25°` grid** (e.g., `41.75_8.5`, `42_8.5`, `42.25_8.5`).
* **Typical Footprint:** `64,000 m × 64,000 m` (`~0.575°` in latitude × `~0.774°` in longitude at 42°N). Because centers are `0.25°` apart (~28 km N–S, ~20 km E–W), adjacent source tiles overlap by more than 50%.

### 2.2 Inside `meta.json` (Generation-2 Example: `42_8.5/meta.json`)
```json
{
  "center": [42.0, 8.5],
  "width": 64000,
  "height": 64000,
  "fuel_resolution": 10,
  "wind_resolution": 400,
  "elevation_resolution": 100,
  "WSEN": [8.112158754362039, 41.71112528183019, 8.886631132547048, 42.28667403094839],
  "LBRT": [0.0, 0.0, 64000.0, 64000.0],
  "WSENLBRT": [
    [8.112158754362039, 41.71112528183019, 8.886631132547048, 42.28667403094839],
    [0.0, 0.0, 64000.0, 64000.0]
  ]
}
```
*(Note: Generation-1 tiles may instead expose `"epsg": 32632` and `"domain": {"xllcorner": ..., "yllcorner": ..., "xurcorner": ..., "yurcorner": ...}` in `meta.json`.)*

### 2.3 Inside `data.nc` (NetCDF3 / NetCDF4)
```text
dimensions:
    ft = 1, fz = 1, fy = 6400, fx = 6400 ;
    wind_dimensions = 2, wind_directions = 2, wind_rows = 160, wind_columns = 160 ;
    nt = 1, nz = 1, ny = 640, nx = 640 ;
variables:
    string domain ;
        domain:SWx = 0.f ; domain:SWy = 0.f ;
        domain:Lx = 64000.f ; domain:Ly = 64000.f ;
        domain:BBoxWSEN = "8.112158754362039,41.71112528183019,8.886631132547048,42.28667403094839" ;
        domain:WSENLBRT = "8.112158754362039,41.71112528183019,8.886631132547048,42.28667403094839,0.0,0.0,64000.0,64000.0" ;
    short fuel(ft, fz, fy, fx) ;       // int16, range 0..255 (e.g. 0..162), 10 m spacing
    short altitude(nt, nz, ny, nx) ;   // int16, meters (e.g. -13..1663), 100 m spacing
    float wind(wind_dimensions, wind_directions, wind_rows, wind_columns) ; // float32, 400 m spacing
```

#### Critical Orientation & Indexing Rules in `data.nc`:
1. **Row / Column Orientation in `data.nc`:**
   * **`row = 0` (`y = 0`) is SOUTH (`SWy`)**, and **`row = N - 1` is NORTH (`SWy + Ly`)**.
   * **`col = 0` (`x = 0`) is WEST (`SWx`)**, and **`col = M - 1` is EAST (`SWx + Lx`)**.
   * *(Warning: Existing `fuel.png` files in old folders were flipped vertically for KML display so `row = 0` was North. In `data.nc`, `row = 0` is South.)*
2. **Wind Tensor Components in `data.nc`:**
   * **Generation-2 (`wind` shape `(2, 2, 160, 160)`):** Already represents the **2×2 Wind Jacobian Tensor** $\mathbf{J}$ mapping synoptic wind $(W_u, W_v)$ to local topographic surface wind $(u, v)$:
     $$\begin{bmatrix} u \\ v \end{bmatrix} = \begin{bmatrix} M_{uu} & M_{uv} \\ M_{vu} & M_{vv} \end{bmatrix} \begin{bmatrix} W_u \\ W_v \end{bmatrix}$$
     Observed statistics in `42_8.5/data.nc`:
     * `wind[0, 0]`: $M_{uu}$ (`min = 0.6051, max = 1.4033, mean = 0.8863`)
     * `wind[0, 1]`: $M_{uv}$ (`min = -0.4221, max = 0.2494, mean = -0.0510`)
     * `wind[1, 0]`: $M_{vu}$ (`min = -0.6554, max = 0.1854, mean = -0.0598`)
     * `wind[1, 1]`: $M_{vv}$ (`min = 0.5553, max = 1.6621, mean = 0.9761`)
   * **Generation-1 (`wind` shape `(2, 8, ny, nx)`):** Stores 8 directional wind responses (`numdir = 8`). In legacy `loadWind.py`, `wind[1]` is `windU` (East) and `wind[0]` is `windV` (North). If converting a Gen-1 tile, extract orthogonal East/North unitary basis components to form $(M_{uu}, M_{uv}, M_{vu}, M_{vv})$.

---

## 3. `.fftile` Spatial Grid & Resolution Specification

### 3.1 Tile Addressing & Geographic Bounds
* **Nominal Core Cell (`0.50° × 0.50°`):**
  * Anchored at south-west corner $(\text{lat}_0, \text{lon}_0)$ where $\text{lat}_0 \in \{-90.0, -89.5, \dots, 89.5\}$ and $\text{lon}_0 \in \{-180.0, -179.5, \dots, 179.5\}$.
  * `wsen_core = [lon0, lat0, lon0 + 0.5, lat0 + 0.5]`
  * Integer grid indices:
    * `lat_idx = int16(round((lat0 + 90.0) * 2.0))` $\in [0, 359]$
    * `lon_idx = int16(round((lon0 + 180.0) * 2.0))` $\in [0, 719]$
  * Canonical filename: `{lat0}_{lon0}.fftile` (formatted cleanly, e.g., `42_8.5.fftile` or `42.0_8.5.fftile`—support lookup by deterministic format `f"{lat0:g}_{lon0:g}.fftile"`).
* **Encoded Image Bounds with Halo (`0.52° × 0.52°`):**
  * Halo margin: $\delta = 0.01^\circ$ ($\approx 1.11\text{ km}$ N–S) on all 4 sides.
  * `wsen_img = [lon0 - 0.01, lat0 - 0.01, lon0 + 0.51, lat0 + 0.51]`
  * Total encoded angular span: $\Delta\text{lat}_{\text{img}} = 0.52^\circ$, $\Delta\text{lon}_{\text{img}} = 0.52^\circ$.

### 3.2 Pixel Dimensions (MCU-Aligned & $\cos(\phi)$ Scaled)
To prevent JPEG `8 × 8` block padding artifacts and maintain ~10 m (fuel), ~50 m (altitude & slope), and ~100 m (wind) metric resolutions worldwide:
* **Height (South-to-North, $0.52^\circ \approx 57.82\text{ km}$, constant at all latitudes):**
  * `wind_h = 576` pixels ($\approx 100.4\text{ m}$ per pixel; $11\text{ px}$ halo + $554\text{ px}$ core + $11\text{ px}$ halo)
  * `slope_h = 1152` pixels ($2 \times 576 \approx 50.2\text{ m}$ per pixel)
  * `alti_h = 1152` pixels ($2 \times 576 \approx 50.2\text{ m}$ per pixel)
  * `fuel_h = 5760` pixels ($10 \times 576 \approx 10.04\text{ m}$ per pixel)
* **Width (West-to-East, scaled by $\cos(\text{lat}_{\text{center}})$ where $\text{lat}_{\text{center}} = \text{lat}_0 + 0.25^\circ$):**
  * `wind_w = max(16, int(round((576 * cos(radians(lat_center))) / 16.0)) * 16)`
  * `slope_w = 2 * wind_w`
  * `alti_w = 2 * wind_w`
  * `fuel_w = 10 * wind_w`
  * *(Example at $\text{lat}_0 = 42.0^\circ\text{N}$, $\text{lat}_{\text{center}} = 42.25^\circ\text{N}$: $\cos(42.25^\circ) = 0.7402 \implies \text{wind\_w} = 432$, $\text{slope\_w} = \text{alti\_w} = 864$, $\text{fuel\_w} = 4320$. Every dimension is an exact multiple of 16!)*

### 3.3 Standard Image Row Orientation in `.fftile`
* **Standardized Convention:** **`row = 0` is SOUTH (`S_img = lat0 - 0.01°`)** and **`row = H - 1` is NORTH (`N_img = lat0 + 0.51°`)**; **`col = 0` is WEST (`W_img = lon0 - 0.01°`)** and **`col = W - 1` is EAST (`E_img = lon0 + 0.51°`)**.
* **Why:** Matches `data.nc` and Cartesian `(x, y)` directly (`col` increases with $+x$ East, `row` increases with $+y$ North) so the C++ solver indexes `row` and `col` without `(H - 1 - row)` inversions. Set `flags bit 0 = 1` (`FLAG_ORIGIN_SW = 0x0001`) in the header to make this explicit.

---

## 4. `.fftile` Binary Format Specification

Each `.fftile` file consists of a **256-byte little-endian binary header** followed immediately by the 4 image byte streams.

### 4.1 Fixed 256-Byte Header Structure (`FFTileHeader`)

```c
#pragma pack(push, 1)
struct FFTileHeader {
    char     magic[8];            // Offset 0:   "FFTILE01" (no null terminator required, exact 8 ASCII bytes)
    int16_t  lat_idx;             // Offset 8:   round((lat0 + 90.0) * 2.0)
    int16_t  lon_idx;             // Offset 10:  round((lon0 + 180.0) * 2.0)
    uint16_t flags;               // Offset 12:  Bit 0 (0x01): Origin is SW (row 0 = South, col 0 = West)
    uint16_t halo_wind_px;        // Offset 14:  Halo thickness in wind pixels (11 for 0.01 deg)

    double   wsen_core[4];        // Offset 16:  [W_core, S_core, E_core, N_core] in degrees (0.50 x 0.50)
    double   wsen_img[4];         // Offset 48:  [W_img,  S_img,  E_img,  N_img]  in degrees (0.52 x 0.52)

    float    m_per_deg_lat;       // Offset 80:  111195.0f (meters per degree latitude)
    float    m_per_deg_lon;       // Offset 84:  111195.0f * cos(lat_center) (meters per degree longitude at center)

    uint16_t fuel_w,  fuel_h;     // Offset 88:  Fuel PNG dimensions (e.g. 4320 x 5760)
    uint16_t alti_w,  alti_h;     // Offset 92:  Altitude JPEG dimensions (e.g. 864 x 1152)
    uint16_t slope_w, slope_h;    // Offset 96:  Single-component Slope dimensions (atlas is 2*slope_w x slope_h)
    uint16_t wind_w,  wind_h;     // Offset 100: Single-component Wind dimensions (atlas is 2*wind_w x 2*wind_h)

    float    alti_min,  alti_max; // Offset 104: Altitude quantization range [m]
    float    sx_min,    sx_max;   // Offset 112: Slope X (dz/dx East) quantization range [m/m]
    float    sy_min,    sy_max;   // Offset 120: Slope Y (dz/dy North) quantization range [m/m]

    float    muu_min,   muu_max;  // Offset 128: Wind Jacobian M_uu range
    float    muv_min,   muv_max;  // Offset 136: Wind Jacobian M_uv range
    float    mvu_min,   mvu_max;  // Offset 144: Wind Jacobian M_vu range
    float    mvv_min,   mvv_max;  // Offset 152: Wind Jacobian M_vv range

    uint32_t fuel_offset,  fuel_size;   // Offset 160: Byte offset & length of 8-bit lossless Fuel PNG
    uint32_t alti_offset,  alti_size;   // Offset 168: Byte offset & length of 8-bit grayscale Altitude JPEG
    uint32_t slope_offset, slope_size;  // Offset 176: Byte offset & length of 8-bit grayscale Slope JPEG (2x1 atlas)
    uint32_t wind_offset,  wind_size;   // Offset 184: Byte offset & length of 8-bit grayscale Wind JPEG (2x2 atlas)

    uint64_t creation_timestamp;  // Offset 192: UTC Unix timestamp (seconds)
    uint32_t header_crc32;        // Offset 200: Optional CRC32 of bytes 0..199 (0 if unused)
    uint8_t  reserved[52];        // Offset 204..255: Zero-padded to exact 256 bytes
};
#pragma pack(pop)
static_assert(sizeof(FFTileHeader) == 256, "FFTileHeader must be exactly 256 bytes");
```
* **Python `struct` format string:** `'<8s2h2H8d2f8H14f8IQIB51x'` (or `'<8s2h2H8d2f8H14f8IQI52s'`, total `256` bytes).

### 4.2 Embedded Image Payloads & Quantization Formulas

All continuous fields are quantized to 8-bit unsigned integers (`0..255`) using:
$$q = \text{clip}\left(\text{round}\left(\frac{v - v_{\min}}{v_{\max} - v_{\min}} \times 255.0\right), 0, 255\right)$$
And dequantized at runtime via precomputed scale $s = (v_{\max} - v_{\min}) / 255.0$ and offset $v_{\min}$:
$$v = v_{\min} + q \cdot s$$
*(If $v_{\max} == v_{\min}$ for a flat/empty field, set $v_{\max} = v_{\min} + 10^{-6}$ during encoding to prevent division by zero.)*

| Payload | Format | Encoded Image Size | Atlas Layout |
| :--- | :--- | :--- | :--- |
| **1. Fuel** | 8-bit Grayscale (`L`) **PNG** (lossless) | `fuel_w × fuel_h` | Direct integer fuel model ID (`0..255`). |
| **2. Altitude** | 8-bit Grayscale (`L`) **JPEG** (quality 92) | `alti_w × alti_h` | Single grid `[0..alti_h-1, 0..alti_w-1]`. |
| **3. Slope** | 8-bit Grayscale (`L`) **JPEG** (quality 92) | `(2 * slope_w) × slope_h` | **Side-by-side (`1 × 2` columns):**<br>• Left `[0..slope_h-1, 0..slope_w-1]` = $S_x = \frac{\partial z}{\partial x}$<br>• Right `[0..slope_h-1, slope_w..2*slope_w-1]` = $S_y = \frac{\partial z}{\partial y}$ |
| **4. Wind Jacobian** | 8-bit Grayscale (`L`) **JPEG** (quality 92) | `(2 * wind_w) × (2 * wind_h)` | **2×2 Quadrant Atlas:**<br>• `[0..wind_h-1, 0..wind_w-1]` = $M_{uu}$<br>• `[0..wind_h-1, wind_w..2*wind_w-1]` = $M_{uv}$<br>• `[wind_h..2*wind_h-1, 0..wind_w-1]` = $M_{vu}$<br>• `[wind_h..2*wind_h-1, wind_w..2*wind_w-1]` = $M_{vv}$ |

> **Why 1-channel Grayscale JPEG Atlases Instead of RGB JPEG?**
> Standard RGB JPEG converts R, G, B into `YCbCr` luminance/chrominance and downsamples `Cb`/`Cr`, causing cross-talk and blurring between independent mathematical components. Packing components into separate quadrants of a **1-channel grayscale JPEG** keeps 100% full luminance resolution with zero channel cross-talk.

---

## 5. Conversion & Rescaling Pipeline from Existing `data.nc` Tiles

### 5.1 Multi-Tile Mosaic & Blending Algorithm
When building a target `0.5° × 0.5°` `.fftile` covering `wsen_img = [W_img, S_img, E_img, N_img]`:

1. **Discover Overlapping Source Tiles:**
   * Scan the input directory of existing `{lat}_{lon}` folders.
   * Read geographic bounds from `meta.json` (`WSEN` array, or `domain` + `epsg` transformed to `EPSG:4326`) or from `data.nc` (`domain:BBoxWSEN`).
   * Select all source tiles whose `[W_src, S_src, E_src, N_src]` intersects `wsen_img`.
2. **Coordinate Mapping from Target `(lat, lon)` to Source Tile Normalized `(u_src, v_src)`:**
   * For each layer's target grid (`fuel`, `alti`, `wind`), generate 1D latitude vector `lats = np.linspace(S_img, N_img, H)` and 1D longitude vector `lons = np.linspace(W_img, E_img, W)` (or pixel-center aligned `S_img + (r + 0.5) * dlat`).
   * **Case A — Source Tile is `WSENLBRT` (Gen-2):**
     $$u_{\text{src}} = \frac{\text{lon} - W_{\text{src}}}{E_{\text{src}} - W_{\text{src}}}, \quad v_{\text{src}} = \frac{\text{lat} - S_{\text{src}}}{N_{\text{src}} - S_{\text{src}}}$$
   * **Case B — Source Tile is `EPSG:xxxx` (Gen-1 UTM):**
     Transform target `(lon, lat)` to `(X_utm, Y_utm)` via `pyproj.Transformer.from_crs("EPSG:4326", f"EPSG:{epsg}", always_xy=True)`:
     $$u_{\text{src}} = \frac{X_{\text{utm}} - \text{SWx}}{L_x}, \quad v_{\text{src}} = \frac{Y_{\text{utm}} - \text{SWy}}{L_y}$$
   * Valid points inside a source tile satisfy $0 \le u_{\text{src}} \le 1$ and $0 \le v_{\text{src}} \le 1$.
3. **Blending Strategy Across Overlapping Source Tiles:**
   * Define a smooth interior weight for each source tile $k$:
     $$w_k(u_{\text{src}}, v_{\text{src}}) = \min(u_{\text{src}}, 1 - u_{\text{src}}, v_{\text{src}}, 1 - v_{\text{src}})$$
     (`w_k` is `0.5` at the center of source tile $k$ and tapers linearly to `0.0` at its borders).
   * **For `fuel` (Categorical `uint8`):**
     * **Never average categorical IDs.** At each target pixel `(r, c)`, select the source tile $k^* = \arg\max_k w_k(u_{\text{src}}, v_{\text{src}})$ (Voronoi / closest-tile-center selection) and sample `fuel` using **nearest-neighbor** indexing.
   * **For `altitude` and `wind` (Continuous `float32`):**
     * Sample each overlapping source tile $k$ using **bilinear interpolation** (`scipy.ndimage.map_coordinates(..., order=1, mode='nearest')`).
     * Compute the weighted average across overlapping tiles using feather weights $w_k^2$:
       $$F_{\text{target}}(r, c) = \frac{\sum_k w_k^2 \cdot F_k(r, c)}{\sum_k w_k^2}$$
     * This eliminates any seam or step discontinuity where overlapping source `data.nc` tiles meet.
4. **Computing Metric Slope Coefficients $(S_x, S_y)$:**
   * Once the unified `altitude` grid (`alti_h × alti_w` at ~50 m resolution) is blended, compute physical rise-over-run in **true meters**:
     $$\Delta y_{\text{meters}} = \frac{0.52^\circ}{H_{\text{alti}}} \times 111195.0$$
     $$\Delta x_{\text{meters}}(r) = \frac{0.52^\circ}{W_{\text{alti}}} \times 111195.0 \times \cos(\text{lat}_r)$$
     $$S_y, S_x = \text{np.gradient}(\text{altitude}, \Delta y_{\text{meters}}, \Delta x_{\text{meters}}(r))$$

---

## 6. C++ ForeFire Reader & Dynamic Paging Architecture

### 6.1 Cartesian Meter Space $\leftrightarrow$ `(lat, lon)` Transform
The simulation defines a reference origin $(\text{lat}_0, \text{lon}_0)$ corresponding to Cartesian $(x_0, y_0) = (0, 0)$ (or the domain's `SWx, SWy` offset for backward compatibility with existing `Init.ff` scripts).

* **Meters `(x, y)` to Degrees `(lat, lon)` (used inside `getValueAt(x, y)`):**
  ```cpp
  constexpr double METERS_PER_DEG = 111195.0;
  constexpr double DEG_TO_RAD     = 0.017453292519943295;

  double lat = origin_lat + (y - origin_y) / METERS_PER_DEG;
  double lon = origin_lon + (x - origin_x) / (METERS_PER_DEG * std::cos(lat * DEG_TO_RAD));
  ```
* **Degrees `(lat, lon)` to Meters `(x, y)` (used for `startFire[lonlat=...]`, KML/JSON output, and atmospheric coupling):**
  ```cpp
  double y = origin_y + (lat - origin_lat) * METERS_PER_DEG;
  double x = origin_x + (lon - origin_lon) * (METERS_PER_DEG * std::cos(lat * DEG_TO_RAD));
  ```

### 6.2 Dynamic Tile Manager (`FFTileManager`)
1. **O(1) Tile Lookup & Halo Hysteresis:**
   * Keep a pointer to `last_tile` (thread-local or per-manager).
   * If `(lat, lon)` falls inside `last_tile->wsen_img` (or within `wsen_core` expanded by half the halo, `0.005°`), reuse `last_tile` immediately without even computing grid indices!
   * Otherwise, compute:
     ```cpp
     int lat_idx = static_cast<int>(std::floor((lat + 90.0) * 2.0));
     int lon_idx = static_cast<int>(std::floor((lon + 180.0) * 2.0));
     ```
   * Look up `(lat_idx, lon_idx)` in the loaded tile table. If not loaded yet, load on demand.
2. **Two-Tier File Resolution (Local Override + Global Store):**
   * When loading tile `(lat_idx, lon_idx)` with `lat0 = lat_idx * 0.5 - 90.0`, `lon0 = lon_idx * 0.5 - 180.0`:
     1. Check **Case Local Directory** first: `{caseDirectory}/{ForeFireDataDirectory}/tiles/{lat0}_{lon0}.fftile` (and optionally `{lat0}_{lon0}_fuel.png` for lightweight local fuel edits).
     2. Fall back to **Global Tile Store**: `{LAYERS_PATH}/{lat0}_{lon0}.fftile`.
3. **In-Memory Decoding (`stb_image.h`):**
   * Read the 256-byte `FFTileHeader`.
   * Read the compressed payload buffer and call `stbi_load_from_memory(buf, size, &w, &h, &channels, 1)` for each of the 4 payloads.
   * **Memory footprint per loaded `0.5° × 0.5°` tile (at 42°N):**
     * `fuel` (`uint8_t`, `4320 × 5760`): `24.8 MB`
     * `altitude` (`uint8_t`, `864 × 1152`): `1.0 MB`
     * `slope` (`uint8_t`, `1728 × 1152`): `2.0 MB`
     * `wind` (`uint8_t`, `864 × 1152`): `1.0 MB`
     * **Total per active 55×43 km tile in RAM:** `~28.8 MB` (keep decoded arrays as `uint8_t` in RAM and apply `min + px * scale` on the 4 bilinearly sampled bytes during lookup!).
4. **Sampling Inside the Loaded Tile:**
   * Given `(lat, lon)` and `wsen_img = [W_img, S_img, E_img, N_img]`:
     ```cpp
     double u = (lon - W_img) / (E_img - W_img); // in [0, 1]
     double v = (lat - S_img) / (N_img - S_img); // in [0, 1]
     ```
   * **Fuel:** Nearest pixel `col = clamp(int(u * fuel_w), 0, fuel_w - 1)`, `row = clamp(int(v * fuel_h), 0, fuel_h - 1)` $\rightarrow$ `fuel_data[row * fuel_w + col]`.
   * **Altitude, Slope ($S_x, S_y$), Wind ($M_{uu}, M_{uv}, M_{vu}, M_{vv}$):** Bilinear interpolation in `(u * W - 0.5, v * H - 0.5)` within each quadrant, followed by `min + val_255 * scale`.
   * **Surface Wind Computation from Synoptic / Atmospheric Wind $(W_u, W_v)$:**
     ```cpp
     double u_surf = M_uu * W_u + M_uv * W_v;
     double v_surf = M_vu * W_u + M_vv * W_v;
     ```

---

## 7. Test Plan on 9 Overlapping Tiles (`3 × 3` Block)

### 7.1 Test Input Dataset
A `3 × 3` block of **9 overlapping legacy `data.nc` tiles** on the `0.25°` grid (for example covering Corsica: latitudes `41.75, 42.0, 42.25` × longitudes `8.5, 8.75, 9.0`, or any `3 × 3` block of `0.25°` tiles downloaded into `test_input_tiles/`):
```text
test_input_tiles/
├── 41.75_8.5/  (data.nc, meta.json)
├── 41.75_8.75/ (data.nc, meta.json)
├── 41.75_9/    (data.nc, meta.json)
├── 42_8.5/     (data.nc, meta.json)
├── 42_8.75/    (data.nc, meta.json)
├── 42_9/       (data.nc, meta.json)
├── 42.25_8.5/  (data.nc, meta.json)
├── 42.25_8.75/ (data.nc, meta.json)
└── 42.25_9/    (data.nc, meta.json)
```
* **Combined Geographic Footprint of the 9 `0.25°` Tiles:**
  * Centers span `[41.75°..42.25°] × [8.5°..9.0°]`.
  * Including their 64×64 km extents (`±0.287°` lat, `±0.387°` lon), the 9 tiles cover roughly **`[41.46°..42.54°] × [8.11°..9.39°]`** (~120 km × 105 km).
* **Target `.fftile` Outputs Generated from this 9-Tile Block:**
  * Complete coverage of the **`2 × 2` block of `0.5° × 0.5°` `.fftile` tiles**:
    1. `41.5_8.0.fftile` (`[41.5°..42.0°] × [8.0°..8.5°]`)
    2. `41.5_8.5.fftile` (`[41.5°..42.0°] × [8.5°..9.0°]`)
    3. `42_8.0.fftile` (`[42.0°..42.5°] × [8.0°..8.5°]`)
    4. `42_8.5.fftile` (`[42.0°..42.5°] × [8.5°..9.0°]`)

### 7.2 Step-by-Step Implementation & Verification Tasks for the Agent

1. **Task 1 — Python Converter & Mosaic Tool (`fftile_gen.py`):**
   * Implement the `.fftile` encoder/decoder and multi-tile `data.nc` mosaic pipeline described in Sections 3, 4, and 5.
   * Run on the 9 overlapping input tiles to produce the `0.5° × 0.5°` `.fftile` files.
   * Provide a `--inspect` / `--unpack` CLI mode that dumps the 256-byte header fields as JSON and extracts `fuel.png`, `altitude.jpg`, `slope.jpg`, and `wind.jpg` for visual inspection.

2. **Task 2 — Quantitative Seam & Reconstruction Validation (`test_fftile_accuracy.py`):**
   * **Header & Size Check:** Verify `sizeof(header) == 256`, `magic == "FFTILE01"`, and verify total `.fftile` size is `< 1.0 MB` per tile.
   * **Fuel Exactness:** Verify 0% classification error on non-overlapping regions and clean Voronoi transitions without synthetic intermediate fuel IDs on overlapping regions.
   * **Compression & Quantization Error Check:**
     * Compare decoded `.fftile` `altitude`, `slope` ($S_x, S_y$), and `wind` ($M_{uu}, M_{uv}, M_{vu}, M_{vv}$) against raw float values from `data.nc`:
       * Altitude RMSE `< 0.5%` of elevation range.
       * Wind Jacobian RMSE `< 0.015` (1.5% of unit wind speed).
       * Slope RMSE `< 0.01` m/m.
   * **Cross-Tile Seam Continuity Test (Crucial):**
     * Sample a dense transect of `(lat, lon)` points crossing the `lat = 42.0°` horizontal boundary (between `41.5_8.5.fftile` and `42_8.5.fftile`) and the `lon = 8.5°` vertical boundary (between `42_8.0.fftile` and `42_8.5.fftile`).
     * Assert that querying `(lat, lon)` in the shared `±0.01°` halo from Tile A vs. Tile B produces near-identical values:
       * $|\Delta \text{altitude}| < 2\text{ m}$
       * $|\Delta M_{uu}|, |\Delta M_{vv}| < 0.02$
       * Exact same `fuel` ID at matching `(lat, lon)` points.

3. **Task 3 — C++ ForeFire `.fftile` Reader & Multi-Tile Propagation Test:**
   * Implement `FFTileLoader` / `FFTileManager` in C++ using `stb_image.h`.
   * Run a test fire simulation ignited at `(lat = 41.99°, lon = 8.49°)` (just South-West of the 4-tile corner `(42.0°, 8.5°)`), propagating North-East under wind $(W_u = 10\text{ m/s}, W_v = 10\text{ m/s})$ across all 4 `.fftile` tiles (`41.5_8.0` $\rightarrow$ `41.5_8.5` / `42_8.0` $\rightarrow$ `42_8.5`).
   * Verify:
     * On-demand lazy loading of the 4 tiles as the fire front crosses `lat = 42.0°` and `lon = 8.5°`.
     * Smooth fire front perimeter across the 4-tile intersection with zero kinks or speed discontinuities.
     * Local override test: place a modified `42_8.5_fuel.png` (with a zero-fuel firebreak strip) in the case's local `tiles/` folder and verify the fire stops along the custom fuel break.
