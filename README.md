<p align="center">
  <img src="./docs/readthedocs/source/_static/forefire.svg" alt="ForeFire Logo" width="400">
</p>

[![Linux CI](https://github.com/forefireAPI/forefire/actions/workflows/main.yml/badge.svg)](https://github.com/forefireAPI/forefire/actions/workflows/main.yml)
[![PyPI](https://img.shields.io/pypi/v/forefire?logo=python&logoColor=white)](https://pypi.org/project/forefire/)
[![DOI](https://joss.theoj.org/papers/10.21105/joss.08680/status.svg)](https://doi.org/10.21105/joss.08680)

**ForeFire** is an open-source wildfire spread simulator written in C++,
developed by CNRS at the [Université de Corse Pascal Paoli](https://www.univ-corse.fr/).
It is part of the [FireCaster](https://firecaster.org/) research portal.

## Try it now, no install

**[Try it in your browser](https://forefireapi.github.io/forefire/)**: the engine
compiled to WebAssembly, running in your tab. It opens on Scenario Studio: set a
start time and place, load the Corsica landscape tiles, and watch the fire spread
hour by hour, or run an ensemble.

<p align="center">
  <a href="https://forefireapi.github.io/forefire/">
    <img src="./docs/readthedocs/source/_static/scenario-studio.jpg" alt="ForeFire Scenario Studio: six hours of fire spread in Corsica, coloured by hour" width="800">
  </a>
</p>

The lighter demo is under [`/classic/`](https://forefireapi.github.io/forefire/classic/):
click the map to light a fire, set the wind, and watch it spread.

<p align="center">
  <a href="https://forefireapi.github.io/forefire/classic/">
    <img src="./docs/readthedocs/source/_static/forefire-demo.gif" alt="A fire spreading east with the wind in the ForeFire classic browser demo" width="600">
  </a>
</p>

Scenario Studio source and build instructions are in
[bindings/wasm/scenario-studio](bindings/wasm/scenario-studio/README.md).

## Run it from Python

On Linux and macOS, `pip install forefire` gives you a self-contained wheel
(NetCDF is bundled) with the `pyforefire` module and the `forefire`
command-line interpreter. NumPy and Matplotlib come with it.

```bash
pip install forefire
```

Save this as `spread.py` and run `python spread.py`. It lights a fire in the
middle of a 10 km square and draws the front every 200 seconds into `fronts.png`:

```python
import matplotlib.pyplot as plt
import pyforefire as forefire
from pyforefire.helpers import printToPathe

ff = forefire.ForeFire()
ff.execute("FireDomain[sw=(0,0,0);ne=(10000,10000,0);t=0]")
ff.addLayer("propagation", "Iso", "propagationModel")
ff.execute("startFire[loc=(5000,5000,0)]")

for _ in range(5):
    ff.execute("step[dt=200]")
    for path in printToPathe(ff.execute("print[]")):
        plt.plot(*path.vertices.T, color="tab:red")

plt.gca().set_aspect("equal")
plt.savefig("fronts.png")
```

Open `fronts.png`: five rings, since `Iso` spreads at the same speed in every
direction. To add fuel, wind and terrain from NumPy arrays, see
[Running from NumPy arrays](https://forefire.readthedocs.io/en/latest/user_guide/python_arrays.html).

The `pyforefire` module drives the same engine as the interpreter. To run
simulations side by side in worker processes, use `ForeFireRunner`; see
[bindings/python/README.md](bindings/python/README.md).

Published wheels are built without MPI. For fire-atmosphere coupling with
Meso-NH, or to tune the build for your CPU, [build from source](#build-from-source).

## Run the web GUI with Docker

The published image includes the interpreter, the web GUI and a real case in
Corsica (`tests/runff`):

```bash
docker run -it --rm -p 127.0.0.1:8000:8000 ghcr.io/forefireapi/forefire:latest
```

`127.0.0.1:` keeps the port on this machine. A bare `-p 8000:8000` publishes
it on every interface, so anyone on the network could send commands to the
server. The [quickstart](https://forefire.readthedocs.io/en/latest/getting_started/quickstart.html#quickstart-docker)
walks through starting the server and running the case from the browser.

## Build from source

The sample data under `tests/` is stored in Git LFS: install Git LFS before
cloning, or `data.nc` will be a small pointer file instead of the dataset.

`install-forefire.sh` (Linux) and `install-forefire-osx.sh` (macOS) install the
dependencies and build; the
[documentation](https://forefire.readthedocs.io/en/latest/) covers the details.
The CMake build is option-driven. The defaults below are what a plain
`cmake -S . -B build` gives you:

| Option | Default | Purpose |
| --- | --- | --- |
| `FOREFIRE_ENABLE_MPI` | `ON` | Enable MPI coupling when MPI is available. |
| `FOREFIRE_ENABLE_NETCDF` | `ON` | Build with NetCDF support (`OFF` compiles in-memory and WASM builds without `libnetcdf`). |
| `FOREFIRE_NATIVE_ARCH` | `ON` | Compile with `-march=native`. Turn off for binaries that must run on other machines. |
| `FOREFIRE_BUILD_CLI` | `ON` | Build the `forefire` command-line executable. |
| `FOREFIRE_BUILD_PYTHON` | `OFF` | Build the `pyforefire` extension module. |
| `FOREFIRE_BUILD_WASM` | `OFF` | Build the Emscripten WebAssembly module (`forefire.mjs` + `forefire.wasm`). |
| `FOREFIRE_STATIC_CORE` | `OFF` | Build the core as a static library instead of `libforefireL`. |
| `FOREFIRE_BUILD_TOOLS` | `ON` | Build the `ANN_test` helper executable. |
| `FOREFIRE_CHECK_LFS` | `ON` | Run the Git LFS integrity check while configuring. |
| `FOREFIRE_BUILD_TESTS` | `ON` | Build the C++ unit tests and register them with CTest. |
| `FOREFIRE_ENABLE_WARNINGS` | `ON` | Compile ForeFire's own sources with `-Wall -Wextra`. |
| `FOREFIRE_WARNINGS_AS_ERRORS` | `OFF` | Fail the build on any compiler warning. Not yet usable repository-wide. |
| `FOREFIRE_SANITIZE` | `""` | Sanitizers to build with, passed to `-fsanitize=` (e.g. `address`). |

Wheel builds (anything driven by `pip`) flip seven of these to the portable defaults:
no MPI, no `-march=native`, static core, Python module on, no tools, no LFS check, no tests.

[TESTING.md](TESTING.md) covers the test suites and the sanitizer build.

### Coupled with Meso-NH

One script takes a bare Linux machine to a running fire-atmosphere case with
[Meso-NH](https://mesonh.cnrs.fr/) (Pedrógão Grande, 4 MPI processes, about 7 min):

```bash
tests/mnh/coupled.bash
```

Options, the live 3D view and the manual setup are in the
[Meso-NH guide](https://forefire.readthedocs.io/en/latest/user_guide/mesonh.html).

## What's in the box

- A C++ engine that tracks the fire front as a set of moving nodes, with
  pluggable rate-of-spread models (Rothermel, several Balbi variants, Farsite,
  and simple wind-driven or isotropic ones for teaching).
- The `forefire` interpreter: `.ff` scripts, an interactive console, and a web
  GUI served by `listenHTTP[]`.
- `libforefireL` for embedding, and Python bindings that take NumPy arrays.
- Landscape from [`.fftile` tiles](https://forefire.readthedocs.io/en/latest/user_guide/tiles.html),
  NetCDF or arrays; checkpoint and restart with `.ffstate`; output to KML,
  GeoJSON, NetCDF and text.
- Two-way coupling with the Meso-NH atmospheric model through MPI.

More: [documentation](https://forefire.readthedocs.io/en/latest/) ·
[FireCaster](https://firecaster.org/) · [changelog](CHANGELOG.md)

## Contributing

Contributions are welcome, especially to the documentation and tutorials, the
Python bindings, and packaging (Docker, pip) across platforms. Read the
[contributing guidelines](CONTRIBUTING.md) for how to report bugs, suggest
features and submit changes. All contributors are expected to follow the
[Code of Conduct](CODE_OF_CONDUCT.md).

## License

ForeFire is licensed under the GNU General Public License v3.0. See
[LICENSE](LICENSE).

## Citation

If you use ForeFire in your work, please cite:

**BibTex**
```bibtex
@article{ForeFireJOSS2025,
  title = {ForeFire: A Modular, Scriptable C++ Simulation Engine and Library for Wildland-Fire Spread},
  volume = {10},
  ISSN = {2475-9066},
  url = {http://dx.doi.org/10.21105/joss.08680},
  DOI = {10.21105/joss.08680},
  number = {116},
  journal = {Journal of Open Source Software},
  publisher = {The Open Journal},
  author = {Filippi, Jean-Baptiste and Baggio, Roberta and Paugam, Ronan and Bosseur, Frédéric and Leblanc, Antonio and Alonso-Pinar, Alberto},
  year = {2025},
  month = dec,
  pages = {8680}
}
```

**Plain Text**
> Filippi, J.-B., Baggio, R., Paugam, R., Bosseur, F., Leblanc, A., & Alonso-Pinar, A. (2025). ForeFire: A Modular, Scriptable C++ Simulation Engine and Library for Wildland-Fire Spread. Journal of Open Source Software, 10(116), 8680. https://doi.org/10.21105/joss.08680
