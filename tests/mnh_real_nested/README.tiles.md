# Run the nested Meso-NH case with FFTile landscapes

This variant keeps the existing two-domain atmospheric configuration and
both geographic ignitions. It creates a **new** working directory under
ignored `local/fftiles/mnh-real-nested/`, symlinks the original atmospheric
inputs and reads the fire landscape from:

```text
https://forefire.univ-corse.fr/fftiles/
```

The original case, `ForeFire/data.nc`, reference outputs and existing results
are left untouched. The working copy contains no `ForeFire/data.nc`. GUI and
recurring plot commands are omitted; physical settings are preserved.

## Run

Build the patched Meso-NH MPI executable and MPI-enabled ForeFire library
first; see [coupling/build notes](../mnh/fftiles-coupling.md).
Meso-NH must load that library through `exe/libForeFire.so`.
Use an MPI launcher matching the runtime used to build both programs.
From the Firefront repository root:

```bash
python3 tests/mnh_real_nested/run_tiles.py \
  --mesonh-root /path/to/meso-nh-6-dev \
  --mpiexec /path/to/matching/mpiexec
```

The default run uses **four MPI ranks and 1,200 simulated seconds**. The
launcher sources the Meso-NH build profile. It prints the new case/log paths,
waits for MPI to finish and writes `validation.json`. Standard-library Python
3.10 or newer is sufficient. No Python simulation binding is involved.

Options:

- `--workdir /new/path`: choose an unused output directory; existing paths
  are rejected.
- `--provider URL`: use another compatible provider or a local tile directory.
- `--duration 120`: shorten the interval in whole minutes; the corresponding
  atmospheric backup time is adjusted. This is a different duration, not a
  baseline result for the original 1,200-second test.
- `--prepare-only`: stage the working case without launching MPI.
- `--validate /completed/path`: rerun validation without a simulation.

Follow progress with `tail -f <case>/OUTPUT_LISTING2` and examine `<case>/run.log`.
No HTTP viewer is started. Native tile downloads use libcurl, without the
browser demo's CORS forwarder.

## Validation

The launcher checks process success, completion to the requested fire time,
finite perimeter coordinates and fire growth. It validates all saved U, V
and BRatio fields for all four MPI partitions, including a nonzero returned
fire tracer. Both nested atmospheric final backups must exist.

The case enables `couplingDiagnosticInterval=60`. ForeFire records summaries
of the **actual arrays crossing its C/Fortran interface**, once per simulated
minute, in `ForeFire/Outputs/coupling.<domain>.csv`. Wind components enter
ForeFire; heat/vapour fluxes return to Meso-NH. Validation requires changing
finite wind values and finite, nonzero returned heat/vapour fluxes. Diagnostics
are disabled in ordinary cases unless this parameter is set.

When available, the model's bundled `ncdump` also checks geographic placement:
the original ignition locations recovered from the first perimeter are
compared with Meso-NH's own LAT/LON mass-point grid. This accounts for the
asymmetric triangle used by `startFire`; averaging its node positions would
not give the requested ignition point. The report explicitly marks this
check skipped if no NetCDF dumper is available.

## Verified case

On 2026-11-11, Meso-NH 6.1 development revision `447a4eda4`, with the companion
geography patch, completed this case twice on Apple Silicon/OpenMPI 5.0.9:

- Four MPI ranks; both nested domains reached 1,200 seconds.
- Fire area grew from **14,400 m² to 586,657.92 m²** (1.44 to 58.67 ha).
- `39.5_-8.5.fftile` was fetched from the public provider. Only the tile
  actually queried by the fires was loaded; other atmospheric-domain tiles
  were not needed by this case.
- Sixteen exchange-summary streams covered both wind components and both
  returned fluxes across four MPI partitions. Nonzero heat/vapour returns
  occurred on partition 4, where both original fires lie.
- All sampled exchange values and saved U/V/BRatio fields were finite.
- Geographic ignition errors were **0.0583 m and 0.0526 m**, limited in part
  by the printed perimeter precision. This compares against the grid map,
  not against the analytic projection at every possible fire location.
- All 21 master perimeter dumps were byte-identical between the run without
  diagnostics and the run with diagnostics. Meso-NH reported zero ERROR and
  zero FATAL messages. The diagnostic run took about 193 seconds wall time.

This validates the supplied case, not bitwise agreement with its old NetCDF
landscape or a scientific calibration of the converted data. The original
fires do not cross an MPI partition boundary during this interval.
