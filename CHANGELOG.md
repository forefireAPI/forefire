# Changelog

All notable changes to ForeFire, one line each. The linked pull request or
issue has the details. Entries that change simulation results are listed first.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Version numbers are the tags in this repository, the values reported by
`forefire -v`, and what `pip install forefire==<version>` resolves.
Entries below v2.5.0 were reconstructed from release notes and commit history.

## [Unreleased]

### Changes results

- Farsite reads the wind in m/s and the slope as a tangent, upslope only: about 5× more wind effect and 4× less slope effect on fuel model 1. Retune old settings. ([#242])
- Farsite matches the FARSITE reference (mbedward/farsite) at the flanks, the back and on slopes; uphill up to a third faster at 30°. ([#269], [#270])
- Time-varying data layers interpolate forwards, past 24 h too. ([#235])
- A front reaching the domain edge no longer floods the arrival map. ([#240])
- A domain built without `loadData` takes its propagation model from the parameters. ([#243])
- The `BMap` layer holds the arrival times again; it returned zeros. ([#229])
- Meso-NH coupling on `.fftile` uses Meso-NH's grid geography (nesting, rotation). The NetCDF coupling is unchanged; a Meso-NH patch is provided. ([#245])

### Added

- `.fftile` landscapes: tiled, loaded on demand, over HTTP with `loadData[provider=…;center=(lon,lat)]` (optional libcurl), no NetCDF needed. Missing tiles are non-burnable. ([#245])
- `.ffstate` checkpoints: `saveState` / `loadState` save and resume a run, and can attach to a Meso-NH domain (`mode=attach`). ([#245])
- Persistent fuel modifiers on tiles (barriers, fuel replacement), kept in `.ffstate`. ([#245])
- Sparse arrival-time storage for tile domains (`arrivalMapSubdivisions`). ([#245])
- Opt-in `normalScheme=slopeCorrectedWeightedMedians` for front normals on slopes; the default is unchanged. ([#284])
- Scenario Studio, a browser scenario editor on Corsica tiles with ensembles and `.ffstate` download, is now the GitHub Pages demo; the previous demo moved to `/classic/`.
- WebAssembly: tile domains, rolling wind fields, perimeter import and ignitions at the current time. ([#245])
- The browser demo can run the Farsite model. ([#274])
- `pyforefire.run_ensemble` and `burn_probability` map an ensemble's burn probability; `dice`, `brier_score` and `reliability` score it against an observed burnt area. ([#172])
- `ForeFireRunner.fork()`, `ForeFireRunner(stdout=, stderr=)` and `pyforefire.arrival_time()`. ([#172])
- `tests/mnh/coupled.bash` goes from a bare Linux machine to a coupled Meso-NH run, with `--compare <branch>`. ([#266])
- `tools/htdocs/mnh3d.html` shows a Meso-NH coupled run live in 3D. ([#271])
- The browser demo shows DFCI grid, water points and tracks from IGN (display only).
- Published releases carry the WebAssembly module and its `SHA256SUMS`. ([#244])
- User Guide pages on Meso-NH coupling, tiles and checkpoints, and every model with its reference.

### Fixed

- Commands that need a domain print an error instead of segfaulting when there is none. ([#229])
- `clear[]` followed by a new domain no longer crashes on the first `startFire`. ([#229])
- `plot[...;filename=*.asc]` writes a valid ESRI ASCII grid. ([#229])
- `setParameter` handles `"""…"""` values, `=` inside them, and empty values. ([#276], [#283])
- `Balbi2020.Cpa` and `FDCellsOutputs=0` take effect; defaults give the same results. ([#276])
- `listenHTTP` reads a POST body sent after the headers, and no longer crashes when started from `pyforefire`. ([#283])
- WebAssembly node IDs are no longer duplicated.
- `step` and `goTo` refuse a missing time or a unit other than seconds, and `startFire` a date without the trailing `Z`, instead of running with it. ([#285])
- Command option values keep everything after the first `=` (`key=a=b` gives `a=b`). ([#285])

## [v2.6.0] — 2026-10-26

### Changes results

- `Rothermel` wind exponent coefficient is 3.59e-4, not 3.59e-3: +6% on `runff` at 5 m/s, much more in loose fuel beds. ([#195])
- `Balbi2020` divides by the surface-to-volume ratio in eq. C7, and its rate of spread is floored at zero downslope with no wind. ([#196])
- A fuel map that misses the domain is stretched over it, like the other layers. ([#220])
- Rate of spread stays finite and decreasing at high dead fuel moisture. ([#158])
- Release builds use `-ffp-contract=off`, so x86, ARM and `-march=native` give the same results.

### Added

- C++ unit tests for every propagation and flux model, `-Wall -Wextra`, and the `FOREFIRE_BUILD_TESTS`, `FOREFIRE_ENABLE_WARNINGS` and `FOREFIRE_WARNINGS_AS_ERRORS` options. ([#156])
- Dead fuel moisture invariant tests and their workflow. ([#158])
- AddressSanitizer CI job and `-DFOREFIRE_SANITIZE=<list>`. ([#180])
- Tests of the HTTP server, and a free-threading stress test that does not pass yet. ([#174], [#176])
- `pyforefire.ForeFireRunner` runs a simulation in its own worker process; a kernel crash raises `WorkerDied`. ([#175])
- `forefire-validate landscape.nc fuels.csv` checks a landscape against its fuel table. ([#190])
- `genForeFireCase.py` is back as a landscape writer. ([#179])
- `-DFOREFIRE_ENABLE_NETCDF=OFF` builds without NetCDF or HDF5. ([#171])
- Experimental WebAssembly build and browser demo. ([#171])
- User Guide page on running from NumPy arrays. ([#189])

### Changed

- `listenHTTP[]` binds to `localhost` by default (`listenHTTP[0.0.0.0:8000]` for the network or Docker), refuses `systemExec`, and serves only the run directory and `tools/htdocs`. For trusted local use. ([#199], [#203])
- Linux wheels are about half the size ([#154]); free-threaded wheels are no longer published ([#155]).
- `import pyforefire` no longer loads matplotlib. ([#193])
- A safe-topology failure no longer ends the process. ([#181])
- Faster propagation loops, same results. ([#221])
- `stb_image_write.h` moved to `third_party/`. ([#192])

### Fixed

- `pyforefire` calls before `FireDomain[...]` raise `RuntimeError` instead of segfaulting. ([#202])
- Crashes in `clear[]` after `loadData` ([#205]) or after a stopped front ([#194]), and with a front reduced to two nodes ([#198]).
- Double free and uninitialised pointer in model destruction. ([#156], [#157])
- `runANN` works and runs in CI. ([#183])
- Thread safety of atom ids, the parameters singleton and string representations. ([#177], [#178])

## [v2.5.0] — 2026-08-11

### Added

- Pip wheels for Linux (x86_64, aarch64) and macOS Apple Silicon: `pip install forefire` gives the `forefire` CLI and `pyforefire`. Intel Macs build from the sdist. ([#151])

### Changed

- Multi-stage Dockerfile. ([#147])
- `install-forefire.sh` uses an option-based `CMakeLists.txt`.

### Fixed

- A double free in `FireFront` cleanup. ([#145])
- `CMakeLists.txt` was missing from the source distribution. ([#152])
- Bugs in `tests/run.bash`. ([#146])

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

[Unreleased]: https://github.com/forefireAPI/forefire/compare/v2.6.0...dev
[v2.6.0]: https://github.com/forefireAPI/forefire/releases/tag/v2.6.0
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
[#195]: https://github.com/forefireAPI/forefire/pull/195
[#202]: https://github.com/forefireAPI/forefire/pull/202
[#199]: https://github.com/forefireAPI/forefire/pull/199
[#179]: https://github.com/forefireAPI/forefire/pull/179
[#181]: https://github.com/forefireAPI/forefire/pull/181
[#182]: https://github.com/forefireAPI/forefire/pull/182
[#189]: https://github.com/forefireAPI/forefire/pull/189
[#190]: https://github.com/forefireAPI/forefire/pull/190
[#192]: https://github.com/forefireAPI/forefire/pull/192
[#221]: https://github.com/forefireAPI/forefire/pull/221
[#203]: https://github.com/forefireAPI/forefire/pull/203
[#205]: https://github.com/forefireAPI/forefire/issues/205
[#171]: https://github.com/forefireAPI/forefire/issues/171
[#172]: https://github.com/forefireAPI/forefire/issues/172
[#265]: https://github.com/forefireAPI/forefire/issues/265
[#242]: https://github.com/forefireAPI/forefire/pull/242
[#270]: https://github.com/forefireAPI/forefire/pull/270
[#266]: https://github.com/forefireAPI/forefire/issues/266
[#229]: https://github.com/forefireAPI/forefire/pull/229
[#235]: https://github.com/forefireAPI/forefire/pull/235
[#240]: https://github.com/forefireAPI/forefire/pull/240
[#243]: https://github.com/forefireAPI/forefire/pull/243
[#244]: https://github.com/forefireAPI/forefire/pull/244
[#269]: https://github.com/forefireAPI/forefire/pull/269
[#271]: https://github.com/forefireAPI/forefire/pull/271
[#274]: https://github.com/forefireAPI/forefire/pull/274
[#276]: https://github.com/forefireAPI/forefire/pull/276
[#283]: https://github.com/forefireAPI/forefire/pull/283
[#284]: https://github.com/forefireAPI/forefire/issues/284
[#285]: https://github.com/forefireAPI/forefire/pull/285
[#245]: https://github.com/forefireAPI/forefire/issues/245
