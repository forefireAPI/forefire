# Running ForeFire coupled with Meso-NH

[Meso-NH](https://mesonh.cnrs.fr) is the French atmospheric research model
(CNRS / Météo-France). Coupled runs load ForeFire as a shared library: at
every step Meso-NH sends wind to the fire and receives heat and vapour fluxes
back. This guide sets the coupling up from scratch and shows how to check
that a change to ForeFire does not alter coupled results.

Tested on Ubuntu 24.04 (WSL) with GCC/gfortran 13.3, OpenMPI 4.1.6 and
Meso-NH 6.0.1. macOS notes, from an older Meso-NH 5.7 setup, are at the end.

## 1. Prerequisites

```bash
sudo apt install build-essential gfortran openmpi-bin libopenmpi-dev \
                 cmake git git-lfs libnetcdf-dev libnetcdf-c++4-dev netcdf-bin
```

Use Bash, not Ksh, for everything below. Meso-NH's profile reads
`SRC_MESONH` before defining it, so do not run with `set -u`.

## 2. Get and build Meso-NH

The 6.0.1 release is on [Zenodo](https://zenodo.org/records/21395048). The
[CNRS GitLab](https://src.koda.cnrs.fr/mesonh/mesonh-code) also works when it
is reachable (it needs Git LFS). Official instructions:
[download](https://mesonh.readthedocs.io/en/mnh-6-0/getting_started/download.html),
[compile](https://mesonh.readthedocs.io/en/mnh-6-0/getting_started/compile.html).

```bash
tar xzf mnh601.tar.gz -C ~/mesonh
cd ~/mesonh/mesonh-code-PACK-MNH-V6-0-1/src
export ARCH=LXgfortran VER_MPI=MPIAUTO OPTLEVEL=O2
./configure
. ../conf/profile_mesonh-LXgfortran-R8I4-MNH-V6-0-1-MPIAUTO-O2
make -j 12
make installmaster
```

The build takes about 15 minutes on 12 cores. Load the profile in every new
shell before running a case: it sets `SRC_MESONH` and `XYZ`, which the test
scripts use.

## 3. Build ForeFire and link it

MPI is on by default in a plain CMake build. Check that the configure output
says MPI was found: without it the coupled run does not work. A warning that
the Git LFS check failed for files under `tests/runff` does not matter here;
the `mnh_*` files must be listed as OK.

```bash
git clone https://github.com/forefireAPI/forefire.git
cd forefire
git lfs pull
cmake -S . -B build
cmake --build build -j 4
```

This produces `lib/libforefireL.so`. Meso-NH opens
`$SRC_MESONH/exe/libForeFire.so` at run time, so link it there:

```bash
ln -sf "$PWD/lib/libforefireL.so" "$SRC_MESONH/exe/libForeFire.so"
```

## 4. Run a coupled case

Two cases ship with ForeFire:

| Case | What it is |
| --- | --- |
| `tests/mnh_real_nested` | Pedrógão Grande 2017, two nested domains, 4 MPI processes, 1200 s of simulation (about 7 min) |
| `tests/mnh_ideal` | Idealised relief, single domain, a few seconds; needs `ncgen` (from `netcdf-bin`) |

```bash
cd tests/mnh_real_nested
bash clean.bash
bash run.bash
```

`run.bash` only checks that `ForeFire/Outputs/ForeFire.0.nc` exists and is
larger than 15 kB. It does not compare against `ForeFire.0.nc.ref`, and the
grid size depends on the Meso-NH version (680×680 on 6.0.1, 666×666 reported
on 6.1). A meaningful check is the comparison in section 5.

`Init.ff` starts `listenHTTP[host=127.0.0.1:8080]`, so port 8080 must be
free.

## 5. Check that a change does not alter coupled results

Run the same case with ForeFire built from `dev` and from the branch under
review, and compare the outputs byte for byte.

1. Build both versions with the same options, each in its own worktree
   (`git worktree add ../ff-pr <branch>`).
2. Make a clean copy of the case for each run.
3. Run `dev`, branch, `dev`, branch, switching the `libForeFire.so` link
   before each run.
4. Compare with `cmp` the files in `ForeFire/Outputs`, `MODEL1`, `MODEL2` and
   the `RUN12.*.FIRE.*.nc` files.

Two runs of the same build already differ in a few places, so the second
`dev` run is what separates noise from a real change:

- `RUN12.*.FIRE.*.nc`: 7 to 9 bytes, the timestamp in the `history`
  attribute and the HDF5 header checksum.
- `MODEL*/output.*.air_density`: meaningless values (NaN, ±10¹⁰) that change
  on every run. This comes from Meso-NH, not from ForeFire.

Everything else must be identical. A file that differs between `dev` and the
branch, but not between the two `dev` runs, is a real difference.

## macOS notes (Meso-NH 5.7)

These come from an earlier setup on Apple Silicon with macOS Ventura and
Meso-NH 5.7. They may no longer apply to 6.x.

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
