# FFTile landscapes with Meso-NH nested grids

The native ForeFire coupling can read fuel and fire terrain from `.fftile`
providers while Meso-NH retains its domain, atmospheric mesh, winds, time
steps, MPI decomposition and flux exchange. No domain expansion is involved.
This integration has native unit coverage. The complete patched Meso-NH MPI
build and installation succeeded on macOS on 2026-11-11. The tile-backed
[real nested case](../../tests/mnh_real_nested/README.tiles.md) completed twice
with four MPI ranks and 1,200 simulated seconds.

## Matching sources

The companion [Meso-NH patch](fftiles-mesonh.patch) targets development
revision `447a4eda4` (MNH 6.1 development, upstream checked 2026-11-11).
It changes three files under `src/LIB/FOREFIRE`:

- `F_ForeFire_Interface.f90`: the optional geography-registration interface;
- `C_ForeFire_Interface.c`: dynamic lookup of the new ForeFire entry point;
- `init_forefiren.f90`: registration before creating the fire domain.

Apply it to a clean checkout before compiling Meso-NH:

```bash
git apply /path/to/firefront/tests/mnh/fftiles-mesonh.patch
```

The local Meso-NH 6 development checkout used for this work already has the
patch applied. Do not apply it twice. The older 5.7 checkout was left alone.
The existing `MNHCreateDomain` C ABI is unchanged. Older ForeFire libraries
ignore the optional registration; newly enabled tile coupling requires it
and fails explicitly if geography is unavailable. Cartesian idealised cases
have no geographic grid and are not enabled for coupled tiles by this patch.

## Coordinates and layers

Meso-NH evaluates its own `SM_LATLON` projection on the current nested grid's
global mass-point axes, `XXHATM_ll` and `XYHATM_ll`. These axes retain the
parent Cartesian frame. All MPI ranks receive the same geographic map for
that nested grid, including its halos; each rank still has its original
local atmospheric domain. This avoids independently shifting each rank to
its local west/south corner. Global coverage also lets the fire master read
ignition geometry outside its local MPI partition.

ForeFire copies the arrays during registration. Tile queries interpolate
latitude/longitude bilinearly on this mass-point map; geographic ignitions
invert the same map. This reproduces projection values at mass points and
interpolates between them: it does not evaluate the analytic projection at
every fire node. The coordinate interpolation error against real Meso-NH
projections has not yet been measured. Queries may extrapolate by one grid
interval to cover atmospheric edges outside the outermost mass points;
farther queries fail instead of silently clamping to a boundary.

Both longitude and latitude are transformed together, including rotation
and antimeridian wrapping. Tile east/north terrain derivatives are mapped
to the Cartesian fire normal through the local geographic Jacobian.
The scalar coordinate helpers are retained for legacy callers; new code
working with a rotated grid must use `getPointFromLonLat` and
`getLatLonFromPoint`, not independent longitude/x and latitude/y conversions.

The fire's `altitude` and `slope` stay backed by the tiles. Incoming Meso-NH
`altitude` writes are redirected to a separate writable
`atmosphericAltitude` layer. `windU`, `windV`, and additional atmospheric
exchange layers stay writable and retain their existing time interpolation.
The tiles' precomputed wind-response matrices are not applied to coupled
winds. Heat/vapour/scalar flux computation and atmospheric-grid aggregation
remain in the existing coupling code.

## Case configuration

Configure the provider **before** Meso-NH creates the domain, in ForeFire's
parameter file read by `MNHInit`:

```text
setParameter[fftilesPath=https://forefire.univ-corse.fr/fftiles/]
setParameter[fftileCacheSize=8]
```

A shared, prepopulated local tile directory is also supported. HTTP uses the
native libcurl provider; it does not use the browser demo's CORS forwarder.
Tiles are loaded on demand within the atmospheric domain. The browser's
standalone `loadData[provider=...;center=...]` command must not be used to
create another domain inside a coupled case. A standalone `fftileCenter`
setting is not needed: the registered Meso-NH map supplies the alignment.

Retain a fuel-property table matching the tile IDs, the chosen propagation
model and explicit coupled flux settings. Replacing `data.nc` does not carry
over any flux-model maps, moisture/temperature fields or other case-specific
layers stored in that file. Configure those separately where required.
For example, existing flux defaults are selected with
`heatFluxDefaultModel` and `vaporFluxDefaultModel`; this integration does not
select different scientific models for a case.

Missing tile coverage is nonburnable. Other HTTP errors and corrupt tiles
are errors. For a reproducible parallel run, populate a shared disk cache or
local provider before starting the model rather than relying on simultaneous
first-time network requests from many ranks.

## macOS build handoff

Activate the intended Fortran/MPI environment yourself, then use the **6.x
checkout**, not a shell profile pointing at the older 5.7 tree. Check:

```bash
command -v gfortran mpifort
mpifort --showme:command
gfortran --version
```

Meso-NH 6.1 already includes the ForeFire interface in its build. The old 5.7
`MNH_FOREFIRE=1.0` switch is not required by the inspected 6.1 makefiles.
The coupling requires double-precision reals and 32-bit integers
(`MNH_REAL=8`, `MNH_INT=4`). A starting configuration, after supplying all
library archives and a macOS-compatible `pyfortool` on PATH, is:

```bash
cd /path/to/meso-nh-6-dev/src
# Use a fresh shell so SRC_MESONH, XYZ and VERSION_XYZ do not refer to 5.7.
export ARCH=LXgfortran VER_MPI=MPIAUTO OPTLEVEL=O2
export MNH_REAL=8 MNH_INT=4
export MNH_FOREFIRE=1.0  # retained for compatibility with older build scripts
# Use the native Python tool instead of the bundled Linux executables.
export VER_PYFORTOOL=PYFORTOOLPERSO
./configure
. ../conf/profile_mesonh
make -j 6
make installmaster
```

The user ran configure and most compilation. After the remaining macOS
linker fixes, `make -j6` and `make installmaster` completed successfully.
The macOS makefile changes are saved in
[fftiles-macos-build.patch](fftiles-macos-build.patch).
The upstream configure script labels Darwin support as untested.

On the inspected Mac, Homebrew's `gfortran` and OpenMPI `mpifort` are present.
The following bundled archives were retrieved through Git LFS on 2026-11-11
and verified against the SHA256 values recorded by the Meso-NH repository:

```text
src/LIB/hdf5-2.2.0.tar.gz
src/LIB/netcdf-c-4.10.1.tar.gz
src/LIB/netcdf-fortran-4.6.4.tar.gz
```

They are ready for the default `CDFAUTO` library build; `libaec-1.1.7` and
`zstd-1.5.7` were verified too. For a fresh checkout, retrieve them with:

```bash
git lfs pull --include='src/LIB/hdf5-2.2.0.tar.gz,src/LIB/netcdf-c-4.10.1.tar.gz,src/LIB/netcdf-fortran-4.6.4.tar.gz' --exclude=''
```

If the server reports a missing object, that archive must be recovered from
an upstream distribution; an LFS pointer cannot be unpacked. `CDFPERSO` is an
alternative only with a compatible external **NetCDF Fortran** installation
and its `nf-config`; NetCDF C alone is insufficient. The bundled Linux `pyfortool`
files remain LFS pointers, and their `aarch64`/`x86_64` directories contain
Linux distribution tools, not a verified native Apple Silicon installation.
The gfortran rules invoke `pyfortool` while preprocessing PHYEX, independently
of whether Python bindings or plotting tools are wanted. It must be available
when those sources are built. A native macOS installation is now prepared in
Firefront's ignored `local/mesonh-build-tools/`, using pyfortool 0.2.10 and
pyfxtran 0.1.5. The build environment adds only its tool launcher to PATH,
leaving the user's Python environment intact. Local parser portability fixes
cover macOS stdio, library linking and finite stack limits. Both Meso-NH
operations (`--addMPPDB_CHECKS` on the failing generated `c2r2_adjust.f90`, and
`--shumanFUNCtoCALL` on `mode_tke_eps_sources.f90`) passed on temporary copies.
The source patch and installation notes are kept beside the local tools.
Merely downloading a Linux executable does not establish macOS compatibility.

In the prepared local checkout, `conf/fftiles-macos.env` supplies the build
flags above without changing the active compiler environment or starting a
build. After activating Fortran in a fresh shell:

```bash
cd /path/to/meso-nh-6-dev
. conf/fftiles-macos.env
cd src
./configure
. ../conf/profile_mesonh
make -j6
make installmaster
```

ForeFire was built separately with MPI and native HTTP enabled and all 13
native test suites passed. To reproduce that library build with the same MPI
runtime:

```bash
cmake -S /path/to/firefront -B /path/to/firefront/local/fftiles/coupled-build \
  -DFOREFIRE_ENABLE_MPI=ON -DFOREFIRE_ENABLE_NETCDF=OFF \
  -DFOREFIRE_ENABLE_TILE_HTTP=ON -DFOREFIRE_CHECK_LFS=OFF
cmake --build /path/to/firefront/local/fftiles/coupled-build -j6
ln -sf /path/to/firefront/lib/libforefireL.dylib /path/to/meso-nh-6-dev/exe/libForeFire.so
```

On macOS, NetCDF configure probes linking the bundled shared HDF5 libraries
need `-Wl,-rpath,${IO_LIBS_PATH}/lib` in addition to `-L${IO_LIBS_PATH}/lib`.
The local Meso-NH `src/Makefile` has this in both NetCDF C/Fortran configure
recipes. Without it, `config.log` reports `Library not loaded:
@rpath/libhdf5_hl.320.dylib` and `cannot run C compiled programs` despite
successful compilation. A small C program calling `H5open()` was compiled
and run successfully with this flag. No HDF5 rebuild or cross-compilation
option is required for that error; rerun make after applying the recipe fix.

If NetCDF Fortran fails with `ld: unknown option: -no_fixup_chains`, check
`command -v ld`. On the inspected Mac, Anaconda's old linker preceded Xcode's
linker on PATH. Clang's configure probe accepted the flag, while gfortran
later invoked Anaconda's linker and failed. The build environment file now
puts Homebrew's compiler tools and Xcode's linker before Anaconda. A small
Fortran shared library reproduced the failure with the original PATH and
linked successfully with this corrected ordering. No linker-flag suppression
or serial-build fallback was applied; MPIAUTO remains selected.

When selecting Xcode's compiler directly through PATH, also export
`SDKROOT="$(/usr/bin/xcrun --sdk macosx --show-sdk-path)"`. Otherwise the MPI C
wrapper can invoke clang without a system-header search root and fail on
`stdio.h` or `sys/resource.h`. The build environment now sets this explicitly.
All four initially affected C sources passed `mpicc -fsyntax-only` with the
corrected environment, including `C_ForeFire_Interface.c`.

The macOS final-link rules omit GNU ld's `--allow-multiple-definition` and
use the portable `-Wl,-rpath,<directory>` spelling. On Darwin, the archive rule
uses Xcode's `ar r`, not GNU's `ar rT`: Apple ar interprets `T` as filename
truncation. The initial archive contained 6,415 members with 3,123 duplicate
names after truncation. The library target now also depends on `Makefile`,
so the next make repacks it atomically from the existing compiled objects.
A two-object test with colliding long filename prefixes preserved both full
names, linked through `mpifort`, and ran successfully with the corrected flags.

ecCodes was compiled with Apple libc++; its Meso-NH link now uses `-lc++`
on Darwin and retains `-lstdc++` on other systems. This resolved the missing
`std::__1` symbols at the final Fortran/MPI link.

ForeFire can omit NetCDF for tile coupling; Meso-NH still needs its own
NetCDF libraries for model input/output. Run a short nested case first,
checking tile addressing at known mass points, fire positions across MPI
boundaries, atmospheric wind updates and returned flux arrays. A complete
supplied tile-backed coupled case has passed; other configurations need their
own validation.


## Completed tile-backed run

The [case launcher](../../tests/mnh_real_nested/run_tiles.py) stages an isolated
copy and validates perimeters, atmospheric fields and direct coupling-array
summaries. In the full four-rank/1,200-second run, fire area increased from
1.44 to 58.67 ha; winds changed and positive heat/vapour fluxes returned to
Meso-NH. Placement against the model's own mass-point map agreed within
0.06 m at printed perimeter precision. The landscape came from the public
HTTP provider, with no `ForeFire/data.nc` in the working case. All 21 master
perimeters matched the first run byte for byte after diagnostics were enabled.
Diagnostics (`couplingDiagnosticInterval`, disabled by default) record finite
array min/max/sum/count values without modifying the exchange.
