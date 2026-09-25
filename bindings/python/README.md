# PyForeFire

<p align="center">
  <img src="https://raw.githubusercontent.com/forefireAPI/forefire/master/bindings/python/pyforefire.svg" alt="PyForeFire Logo" width="300">
</p>

**PyForeFire** provides Python bindings for [ForeFire](https://github.com/forefireAPI/forefire),
an open-source wildfire simulation engine written in C++ and developed by CNRS
at the Université de Corse Pascal Paoli.

The distribution is named `forefire` on PyPI; the importable module is
`pyforefire`.

---

## Installation

```bash
pip install forefire
```

Wheels are published for Linux (x86_64, aarch64) and macOS (Apple Silicon and
Intel), on CPython 3.9 and newer. They are self-contained: NetCDF and its own
dependencies are bundled inside the wheel, so there is nothing to install
beforehand and nothing to configure.

Installing also puts the `forefire` command-line interpreter on your `PATH`:

```bash
forefire -v
```

### What the published wheels do not include

Wheels are built for portability, which means they deliberately leave out two
build-time features:

- **MPI coupling** is disabled, so wheels cannot drive coupled fire-atmosphere
  runs with MesoNH.
- **CPU-specific optimisation** (`-march=native`) is off, so the binary runs on
  any machine of the same architecture rather than only on the build machine.

If you need either, build from source (below).

### Building from source

Any platform without a published wheel — Windows, musl-based Linux, or an
unusual architecture — falls back to compiling the sdist, which needs a C++
compiler, CMake ≥ 3.15, and the NetCDF C and legacy C++4 libraries:

```bash
# Debian/Ubuntu
sudo apt install build-essential cmake libnetcdf-dev libnetcdf-c++4-dev
# Fedora/RHEL
sudo dnf install gcc-c++ cmake netcdf-devel netcdf-cxx4-devel
# macOS
brew install cmake netcdf netcdf-cxx

pip install forefire --no-binary forefire
```

To build with MPI support and native optimisation, pass the CMake options
through:

```bash
pip install forefire --no-binary forefire \
  --config-settings=cmake.define.FOREFIRE_ENABLE_MPI=ON \
  --config-settings=cmake.define.FOREFIRE_NATIVE_ARCH=ON
```

If NetCDF lives somewhere CMake does not look, point at it with
`--config-settings=cmake.define.NETCDF_HOME=/path/to/netcdf` (and
`NETCDF_CXX_HOME` if the C++4 API is installed separately).

---

## Usage

### Verifying the installation

```python
import pyforefire as forefire

ff = forefire.ForeFire()
ff.execute("FireDomain[sw=(0.,0.,0.);ne=(300.,200.,0.);t=0.]")
print("PyForeFire installed and domain created successfully.")
```

If this runs without an `ImportError` or linking error, your installation is
working. *Note: you may see warnings about missing fuel tables, which is
expected at this stage.*

### Running a simple simulation

This example starts a fire in the centre of a domain and runs it for 1000
seconds.

```python
import pyforefire as forefire

ff = forefire.ForeFire()

# 1. Define a 10km x 10km simulation domain
sim_shape = (10000, 10000)
ff.execute(f'FireDomain[sw=(0,0,0);ne=({sim_shape[0]},{sim_shape[1]},0);t=0]')

# 2. Set a simple propagation model (isotropic, i.e. a perfect circle)
ff.addLayer("propagation", "Iso", "propagationModel")

# 3. Start a fire in the center of the domain
ff.execute(f'startFire[loc=({sim_shape[0]/2},{sim_shape[1]/2},0.0)]')

# 4. Run the simulation forward by 1000 seconds
ff.execute("step[dt=1000]")

# 5. Print the state of the fire front to the console
print(ff.execute("print[]"))
```

This produces text output describing the location of the fire front nodes.

To generate a `circle.kml` file for visualization in Google Earth, set the
`dumpMode` parameter before the final print command:

```python
ff["dumpMode"] = "kml"
ff.execute("print[circle.kml]")
```

### More advanced examples

For examples that use real-world data (fuel, topography, wind), see the scripts
in the [`tests/python/`](https://github.com/forefireAPI/forefire/tree/master/tests/python)
directory of the main repository.

### Validating a landscape file

A landscape `.nc` and its fuel table have to agree: every fuel index in the
raster must be defined in the table, or the simulation fails or produces wrong
results without saying why. The `forefire-validate` command checks this up
front:

```bash
forefire-validate landscape.nc fuels.csv
```

It reports any fuel index present in the raster but missing from the table,
flags a fuel/elevation shape mismatch, and warns when elevation or wind is
absent. Reading the `.nc` needs the `netCDF4` package (`pip install netCDF4`);
it is not pulled in by the wheel automatically.

---

## Development

The Python bindings are built from the repository root, together with the C++
core:

```bash
git clone https://github.com/forefireAPI/forefire.git
cd forefire
pip install -e .
```

Re-run that command after touching `_pyforefire.cpp` or the C++ core. If you
iterate often, install the build requirements once and let scikit-build-core
recompile on import instead:

```bash
pip install scikit-build-core pybind11
pip install -e . --no-build-isolation --config-settings=editable.rebuild=true
```

To build a wheel without installing it:

```bash
pip wheel . -w dist/
```

The build is driven by [scikit-build-core](https://scikit-build-core.readthedocs.io/),
configured in the root `pyproject.toml`; the extension module target itself
lives in the root `CMakeLists.txt` behind `FOREFIRE_BUILD_PYTHON`.

### Smoke testing a built wheel

```bash
python tests/python/test_wheel.py
```

Run this against an installed wheel rather than from a build tree: it checks
that the extension loads, that its bundled NetCDF resolves, and that a trivial
simulation advances.

---

## Troubleshooting

- **`NetCDF not found` while building from source:** install the *two* NetCDF
  packages listed above. The C library alone is not enough; ForeFire's
  `DataBroker` includes the legacy C++4 header `<netcdf>`, which ships in
  `libnetcdf-c++4-dev` / `netcdf-cxx4-devel` / `netcdf-cxx`.
- **`Illegal instruction` after copying a self-built install to another
  machine:** it was compiled with `-march=native`. Rebuild with
  `FOREFIRE_NATIVE_ARCH=OFF`, or use the published wheel.

---

## License

ForeFire is licensed under the GNU General Public License v3.0. See the
[LICENSE](https://github.com/forefireAPI/forefire/blob/master/LICENSE) file.

---

## Project URLs

- **Homepage:** [https://forefire.univ-corse.fr/](https://forefire.univ-corse.fr/)
- **Repository:** [https://github.com/forefireAPI/forefire](https://github.com/forefireAPI/forefire)
- **Documentation:** [https://forefire.readthedocs.io/en/latest/](https://forefire.readthedocs.io/en/latest/)
