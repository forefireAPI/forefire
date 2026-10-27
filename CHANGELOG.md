# Changelog

All notable changes to ForeFire are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Version numbers are the tags in this repository and the values reported by
`forefire -v`; they are also what `pip install forefire==<version>` resolves.

Entries below v2.5.0 were reconstructed from the release notes and the commit
history, so they summarise each release rather than list every change. The
`Full Changelog` link on each version gives the complete commit range.

## [Unreleased]

Merged since v2.5.0, not yet released.

### Added

- C++ unit tests covering every propagation and flux model, built by default
  and registered with CTest. They exercise the models one call at a time,
  which the `runff` regression test cannot reach. ([#156])
- A dead fuel moisture invariant suite (`tests/python/test_moisture_invariants.py`)
  and the `invariants.yml` workflow that runs it. Unlike `runff` it holds no
  reference data: every assertion follows from the published spread equations,
  so it stays valid across recalibration. ([#158])
- A blocking AddressSanitizer job, and `-DFOREFIRE_SANITIZE=<list>` to build
  with `-fsanitize=<list>` on the compile line, the executables and the shared
  library. ([#180])
- Characterisation tests pinning the HTTP command server's current behaviour.
  ([#174])
- A concurrency stress test for free-threaded CPython
  (`tests/python/test_threading.py`). It does not pass yet: it is the failing
  test for the shared-state work in [#175]. ([#176])
- `-Wall -Wextra` on ForeFire's own sources, with `FOREFIRE_ENABLE_WARNINGS`
  and `FOREFIRE_WARNINGS_AS_ERRORS` to control them. NetCDF's headers are
  included as system headers so their warnings do not appear. ([#156])
- `FOREFIRE_BUILD_TESTS` (default on outside wheel builds) to build the unit
  tests. ([#156])
- `TESTING.md` now documents every suite, how to run it, and which ones CI
  validates.
- `pyforefire.ForeFireRunner`, a `ForeFire` that runs in its own worker
  process, so several simulations in one Python process no longer share
  state, and a crash in the kernel raises `pyforefire.WorkerDied` instead of
  ending Python. `ForeFire()` itself is unchanged. ([#175])
- `forefire-validate landscape.nc fuels.csv` checks a landscape against its
  fuel table before a run: fuel indices missing from the table, a fuel and
  elevation shape mismatch, and missing elevation or wind. ([#190])
- `tools/preprocessing/genForeFireCase.py` is back as a general landscape
  writer. Its 3-D and 4-D fields are written in the right axis order, it uses
  `netCDF4` instead of the deprecated `scipy.io.netcdf`, and it checks the
  required `parametersProperties` keys before writing. ([#179])
- A User Guide page on driving a simulation from NumPy arrays, with no
  landscape file and no GIS. ([#189])
- `-DFOREFIRE_ENABLE_NETCDF=OFF` builds the engine without NetCDF or HDF5,
  the first step towards a WebAssembly build and a slim wheel. Commands that
  read or write NetCDF files report that the build has no NetCDF support.
  The default stays on. ([#171])
- An experimental WebAssembly build: `emcmake cmake` compiles the engine
  without NetCDF into an ES module whose embind API mirrors `pyforefire`, with
  typed arrays in place of NumPy arrays. `FOREFIRE_BUILD_CLI` and
  `FOREFIRE_BUILD_WASM` control the two executables. See
  `bindings/wasm/README.md`. ([#171])
- A browser demo of the WebAssembly build (`bindings/wasm/index.html`): pick
  a square of ground on a map, and the page builds fuel and elevation layers
  from satellite and terrain tiles, runs the fire in a worker, and plays it
  back in 2D or on 3D terrain. Explore mode lights a fire where you click;
  Research mode exposes the model, its tuning factors, the numerics, and the
  fuel moisture and classes. ([#171])
- The browser demo colours the burnt area by arrival time, with labelled
  isochrones, and the fire front by its rate of spread. ([#171])

### Fixed

- `pyforefire` calls that read the domain (`ff[key]`, `addLayer`,
  `addScalarLayer`, `addIndexLayer`, `getDoubleArray`) segfaulted Python when
  run before `FireDomain[...]`. They now raise `RuntimeError`. ([#202])
- `clear[]` after `loadData` freed the propagative layer twice and crashed.
  The data broker lists that layer but no longer deletes it; the domain does.
  ([#205])
- Rate of spread stayed finite and decreasing at high dead fuel moisture, and
  `DataBroker` no longer serves `moisture` through the five-slot getter, which
  returned a neighbouring property. ([#158])
- The model properties array was freed twice on destruction. ([#157])
- `~ForeFireModel` freed an uninitialised pointer when construction had not
  reached the allocation. ([#156])
- `runANN` had failed on its second line since it was committed: it diffed
  against `result.txt.ref`, a file that is not in the repository. It now checks
  the root mean squared error that `ANN_test` already computes, which does not
  depend on the last digit of a machine-specific reference, and it runs in CI.
  ([#183])
- The `ForeFireAtom` instance counter is now atomic and the `SimulationParameters`
  singleton is initialised safely, so two threads no longer race for ids or
  construct the singleton twice. ([#177])
- `StringRepresentation` kept its output buffer, current level and GeoJSON
  cursor in file-scope globals shared by every instance. They are now members.
  ([#178])
- A fire front reduced to two nodes kept a deleted node as its head, and the
  next burning scan crashed the simulation. ([#198])
- `clear[]` no longer crashes after a simulation whose fire front stopped
  everywhere, for instance an ignition in non-burnable fuel. ([#194])
- `Balbi2020` multiplied by the fuel surface-to-volume ratio instead of
  dividing by it in eq. C7, which drove the radiant fraction to zero. It now
  matches `Balbi2020.py` in `wildfire_ROS_models`, and its rate of spread is
  floored at zero on a downslope with no wind. ([#196])
- A fuel map loaded with `loadData` that does not overlap the domain is now
  stretched over it, as the warning says and as the other layers already were.
  It was left at its own extent, so the whole domain read the fuel of its first
  cell. ([#220])

### Changed

- Linux wheels are built against a NetCDF without DAP and HDF4, roughly halving
  the wheel. ([#154])
- Free-threaded (`cp314t`) wheels are no longer published, because the core is
  not yet thread-safe; CPython 3.14 is declared supported. ([#155])
- `import pyforefire` no longer loads matplotlib; it is imported only when
  `printToPathe` or `plot_simulation` is called. `plt`, `cm`, `mpath`,
  `mpatches` and `ListedColormap` are no longer reachable as `pyforefire`
  attributes. ([#193])
- `listenHTTP[]` binds to `localhost` by default and to the host given in
  `listenHTTP[host:port]`; use `listenHTTP[0.0.0.0:8000]` for the network
  (and inside Docker), which prints a warning.
  `systemExec` is refused over HTTP, the static file server only serves the run
  directory and `FOREFIREHOME/tools/htdocs`, and commands from another site or
  host name are refused. Other commands can still read and write files, so the
  server is meant for trusted local use. ([#199], [#203])
- A safe-topology failure in a step no longer ends the process; the command
  reports an error and the coupled run continues. ([#181])
- `stb_image_write.h` moved to `third_party/`, with its version, origin and
  MIT / public-domain licence recorded. ([#192])

## [v2.5.0] — 2026-08-11

### Added

- **Pip-installable wheels** for Linux (x86_64, aarch64) and macOS on Apple
  Silicon, CPython 3.9 and newer. `pip install forefire` gives both the
  `forefire` command-line interpreter and the `pyforefire` module, with NetCDF
  bundled inside the wheel. Wheels are built without MPI and without
  `-march=native`. There is no Intel macOS wheel — the `macos-13` runner was
  retired before the release, so Intel Macs build from the source
  distribution. ([#151])

### Changed

- The Dockerfile is a multi-stage build, and only `libforefireL` is built in
  the builder stage. ([#147])
- The `install-forefire.sh` build is driven by an option-based `CMakeLists.txt`
  (`FOREFIRE_ENABLE_MPI`, `FOREFIRE_NATIVE_ARCH`, `FOREFIRE_BUILD_PYTHON`,
  `FOREFIRE_STATIC_CORE`, `FOREFIRE_BUILD_TOOLS`, `FOREFIRE_CHECK_LFS`).

### Fixed

- A double free in `FireFront` cleanup. ([#145])
- `CMakeLists.txt` was matched by `.gitignore`, so it was missing from the
  source distribution. ([#152])
- Bugs in `tests/run.bash`, including a `runANN` typo and an unreachable
  `./clean.bash`. ([#146])

**Full Changelog**: <https://github.com/forefireAPI/forefire/compare/v2.4.2...v2.5.0>

## [v2.4.2] — 2025-11-27

- Simplified the installation process, following review comments on the JOSS
  submission, and corrected the installation instructions.
- Aligned the coupling with
  [PACK-MNH-V5-7-2](https://src.koda.cnrs.fr/mesonh/mesonh-code/-/releases/PACK-MNH-V5-7-2).
- Aligned the repository with the accepted JOSS paper
  ([10.21105/joss.08680](https://doi.org/10.21105/joss.08680)).

**Full Changelog**: <https://github.com/forefireAPI/forefire/compare/v2.1.122...v2.4.2>

## [v2.1.122] — 2025-09-16

Tagged, but never published as a GitHub release.

- Docker images published to the GitHub container registry, and tested in CI.
- Python bindings reworked, with the example from [#103] applied.
- macOS CI runs the test script.

**Full Changelog**: <https://github.com/forefireAPI/forefire/compare/v2.0...v2.1.122>

## [v2.0] — 2025-06-05

The V2 release: the HTTP interface, and alignment with Meso-NH V4.7.2.

### Added

- The built-in **HTTP command server and web UI** (`listenHTTP[]`, or
  `forefire -l`), serving a map view of the simulation.
- The **Read the Docs documentation site**, built with Sphinx, Breathe and
  Doxygen.
- `install-forefire.sh`, with `-y` to add `forefire` to `PATH` and set
  `FOREFIREHOME`.
- A working Dockerfile, and GitHub Actions for Linux and macOS.
- The `RothermelAndrews2018` propagation model. ([#34])

### Changed

- CMake replaces SCons throughout; references to SCons were removed.

### Fixed

- The `-lnetcdf_c++4` link failure. ([#26])
- A division by zero in `setArrivalTime`/`getArrivalTime`. ([#36])

**Full Changelog**: <https://github.com/forefireAPI/forefire/compare/v1.2...v2.0>

## [v1.2] — 2024-01-16

### Added

- The `geojson` dump mode.

## [v1.1.10] — 2022-10-25

### Changed

- CMake became the default build system. ([#9])

## [v1.1.0] — 2022-09-28

Tagged before the sources were moved into `src/`, so that this point in the
repository stays easy to return to.

[Unreleased]: https://github.com/forefireAPI/forefire/compare/v2.5.0...dev
[v2.5.0]: https://github.com/forefireAPI/forefire/releases/tag/v2.5.0
[v2.4.2]: https://github.com/forefireAPI/forefire/releases/tag/v2.4.2
[v2.1.122]: https://github.com/forefireAPI/forefire/releases/tag/v2.1.122
[v2.0]: https://github.com/forefireAPI/forefire/releases/tag/v2.0
[v1.2]: https://github.com/forefireAPI/forefire/releases/tag/v1.2
[v1.1.10]: https://github.com/forefireAPI/forefire/releases/tag/v1.1.10
[v1.1.0]: https://github.com/forefireAPI/forefire/releases/tag/v1.1.0

[#9]: https://github.com/forefireAPI/forefire/issues/9
[#26]: https://github.com/forefireAPI/forefire/pull/26
[#34]: https://github.com/forefireAPI/forefire/pull/34
[#36]: https://github.com/forefireAPI/forefire/pull/36
[#103]: https://github.com/forefireAPI/forefire/issues/103
[#145]: https://github.com/forefireAPI/forefire/pull/145
[#146]: https://github.com/forefireAPI/forefire/pull/146
[#147]: https://github.com/forefireAPI/forefire/pull/147
[#151]: https://github.com/forefireAPI/forefire/pull/151
[#152]: https://github.com/forefireAPI/forefire/pull/152
[#154]: https://github.com/forefireAPI/forefire/pull/154
[#155]: https://github.com/forefireAPI/forefire/pull/155
[#156]: https://github.com/forefireAPI/forefire/pull/156
[#157]: https://github.com/forefireAPI/forefire/pull/157
[#158]: https://github.com/forefireAPI/forefire/pull/158
[#174]: https://github.com/forefireAPI/forefire/pull/174
[#175]: https://github.com/forefireAPI/forefire/issues/175
[#176]: https://github.com/forefireAPI/forefire/pull/176
[#177]: https://github.com/forefireAPI/forefire/pull/177
[#178]: https://github.com/forefireAPI/forefire/pull/178
[#180]: https://github.com/forefireAPI/forefire/pull/180
[#183]: https://github.com/forefireAPI/forefire/pull/183
[#193]: https://github.com/forefireAPI/forefire/pull/193
[#198]: https://github.com/forefireAPI/forefire/pull/198
[#197]: https://github.com/forefireAPI/forefire/pull/197
[#194]: https://github.com/forefireAPI/forefire/pull/194
[#196]: https://github.com/forefireAPI/forefire/issues/196
[#220]: https://github.com/forefireAPI/forefire/pull/220
[#202]: https://github.com/forefireAPI/forefire/pull/202
[#199]: https://github.com/forefireAPI/forefire/pull/199
[#179]: https://github.com/forefireAPI/forefire/pull/179
[#181]: https://github.com/forefireAPI/forefire/pull/181
[#182]: https://github.com/forefireAPI/forefire/pull/182
[#189]: https://github.com/forefireAPI/forefire/pull/189
[#190]: https://github.com/forefireAPI/forefire/pull/190
[#192]: https://github.com/forefireAPI/forefire/pull/192
[#203]: https://github.com/forefireAPI/forefire/pull/203
[#205]: https://github.com/forefireAPI/forefire/issues/205
[#171]: https://github.com/forefireAPI/forefire/issues/171
