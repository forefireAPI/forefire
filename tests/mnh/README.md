# Running ForeFire coupled with Meso-NH

[Meso-NH](https://mesonh.cnrs.fr) is the French atmospheric research model
(CNRS / Météo-France). Coupled runs load ForeFire as a shared library: at
every step Meso-NH sends wind to the fire and receives heat and vapour fluxes
back.

Tested on Ubuntu 24.04 (WSL) with GCC/gfortran 13.3, OpenMPI 4.1.6 and
Meso-NH 6.0.1. macOS notes, from an older Meso-NH 5.7 setup, are at the end.

For tile-backed landscapes and nested-grid geography, see the
[FFTile coupling notes and source patch](fftiles-coupling.md).

## 1. Run the script

From a ForeFire clone, in Bash:

```bash
tests/mnh_coupled.bash
```

It goes from a bare Linux machine to a finished coupled run, and skips
whatever is already done, so running it again is cheap:

1. Checks the prerequisites and, if any are missing, prints the
   `sudo apt install` line for them. It never runs `sudo` itself.
2. Downloads Meso-NH 6.0.1 from [Zenodo](https://zenodo.org/records/21395048)
   (190 MB) and checks its MD5. `--tarball FILE` uses a copy you already have.
3. Builds Meso-NH: about 15 minutes on 12 cores.
4. Builds the ForeFire library with MPI, from the clone the script is in.
5. Links it as `$SRC_MESONH/exe/libForeFire.so`.
6. Runs `tests/mnh_real_nested` in a fresh copy: Pedrógão Grande 2017, two
   nested domains, 4 MPI processes, about 7 minutes.

To watch the run in 3D (terrain, fire front, plume, wind), open
<http://localhost:8080/mnh3d.html> while it runs (it loads three.js from a
CDN, so the browser needs internet; use one tab at a time).

Everything goes under `~/forefire-mnh` (`--workdir` or `FF_MNH_WORKDIR` to
change it), about 10 GB: Meso-NH, the ForeFire builds, `logs/` and one
directory per run under `runs/`. `--help` lists the options.

The packages it asks for on Ubuntu:

```bash
sudo apt install build-essential gfortran openmpi-bin libopenmpi-dev \
                 cmake git git-lfs curl libnetcdf-dev libnetcdf-c++4-dev
```

## 2. Check that a change does not alter coupled results

```bash
tests/mnh_coupled.bash --compare my-branch
```

`--compare` takes a branch, tag or commit of the same clone, or a directory
with another ForeFire source tree. The script builds both, runs the case four
times (this clone, the other one, this clone, the other one), switching the
`libForeFire.so` link before each run, and compares byte for byte the files
in `ForeFire/Outputs`, `MODEL1`, `MODEL2` and the `RUN12.*.FIRE.*.nc` files.
It takes about 30 minutes and exits non-zero on a real difference. Leave
`mnh3d.html` closed during `--compare`, as it sends commands that run
alongside the time step.

Two runs of the same build already differ in a few places, so the second run
of this clone is what separates noise from a real change:

- `RUN12.*.FIRE.*.nc`: 7 to 9 bytes, the timestamp in the `history`
  attribute and the HDF5 header checksum.
- `MODEL*/output.*.air_density`: meaningless values (NaN, ±10¹⁰) that change
  on every run. This comes from Meso-NH, not from ForeFire.

Everything else must be identical. A file that differs between the two
builds, but not between the two runs of this clone, is a real difference.
A file that differs between the two runs of this clone and is not in the list
above is reported as unexpected noise.

## 3. What the script relies on

What follows explains the setup for anyone doing it by hand or debugging it.
Official Meso-NH instructions:
[download](https://mesonh.readthedocs.io/en/mnh-6-0/getting_started/download.html),
[compile](https://mesonh.readthedocs.io/en/mnh-6-0/getting_started/compile.html).
The [CNRS GitLab](https://src.koda.cnrs.fr/mesonh/mesonh-code) also has the
sources when it is reachable (it needs Git LFS).

- **Meso-NH build.** `ARCH=LXgfortran VER_MPI=MPIAUTO OPTLEVEL=O2 ./configure`
  in `src/`, then load the profile it writes in `conf/`, then `make` and
  `make installmaster`. The profile sets `SRC_MESONH` and `XYZ`, which the
  test cases use, so load it in every new shell before running one by hand.
  It reads `SRC_MESONH` before defining it, so do not use `set -u` around it.
- **ForeFire build.** `cmake -S . -B build -DFOREFIRE_ENABLE_MPI=ON`. The
  configuration summary must say `MPI .......... ON`: without MPI the coupled
  run does not work. Whatever the build directory, CMake writes
  `lib/libforefireL.so` into the source tree, so two builds of one tree
  overwrite each other's library. The script copies it to
  `forefire/<name>/libforefireL.so` in its work directory before linking.
- **The link.** Meso-NH opens `$SRC_MESONH/exe/libForeFire.so` at run time:

  ```bash
  ln -sf /path/to/libforefireL.so "$SRC_MESONH/exe/libForeFire.so"
  ```

- **Case data.** The `mnh_*` inputs are in Git LFS. If they are short text
  stubs, the script runs `git lfs pull` for the case.
- **Port 8080.** `mnh_real_nested/ForeFire/Init.ff` starts
  `listenHTTP[host=127.0.0.1:8080]`, so the port must be free; the script
  stops if it is not. With `FOREFIREHOME` set to the ForeFire clone, which
  the script does, that server also serves the pages in `tools/htdocs`.
- **What `run.bash` checks.** Only that `ForeFire/Outputs/ForeFire.0.nc`
  exists and is larger than 15 kB. It does not compare against
  `ForeFire.0.nc.ref`, and the grid size depends on the Meso-NH version
  (680×680 on 6.0.1, 666×666 reported on 6.1). The comparison in section 2
  is the meaningful check.

The other coupled case, `tests/mnh_ideal` (idealised relief, single domain, a
few seconds), is not run by the script. With the profile loaded and the link
in place: `cd tests/mnh_ideal && bash clean.bash && bash run.bash`. It needs
`ncgen`, from `netcdf-bin`.

## macOS notes (Meso-NH 5.7)

The script targets Linux. These notes come from an earlier setup on Apple
Silicon with macOS Ventura and Meso-NH 5.7. They may no longer apply to 6.x.

Install `gcc`, `gfortran` and `open-mpi` with Homebrew. On 5.7, also
`export MNH_FOREFIRE=1.0` before `./configure`. The library is
`lib/libforefireL.dylib`; link it as `libForeFire.so` in `$SRC_MESONH/exe`
all the same.

### libxml2 error when configuring NetCDF 4.9.2

```bash
brew install autoconf automake libtool
cd $SRC_MESONH/src/LIB/netcdf-c-4.9.2/
```

In `configure.ac`, around line 1222, change

```plaintext
if test "x$ISOSX" = xyes && test "x$have_libxml2" = xno; then
```

to

```plaintext
if test "x$ISOSX" = xyes && "x$have_libxml2" = xno; then
```

then run `autoreconf`.

### NetCDF C++ configuration hangs

`$SRC_MESONH/src/LIB/netcdf-cxx4-4.3.1/VERSION` gets a bad version string
(e.g. `-n 4.4.3`) and must be empty. While `make` runs in one terminal, empty
it from another as soon as it appears, after HDF5 and the NetCDF C
configuration:

```bash
cat VERSION; echo "" > VERSION
```

### `mpirun` loops forever

The case-insensitive filesystem confuses `Mpirun` with `mpirun`:

```bash
cd $SRC_MESONH/bin
mv Mpirun NoMpirun
```
