# PyForeFire

<p align="center">
  <img src="./pyforefire.svg" alt="PyForeFire Logo" width="300">
</p>


**PyForeFire** provides Python bindings for the ForeFire library, enabling users to access ForeFire’s functionality directly from Python. The bindings link against the precompiled ForeFire library and include support for NetCDF and other dependencies.

## Overview

The PyForeFire package is designed to:
- Expose core ForeFire functionality to Python via pybind11.
- Link against a precompiled ForeFire library located in the top-level `lib/` directory.
- Optionally incorporate NetCDF support by detecting either a static NetCDF installation (via `SRC_MESONH` and `XYZ`) or a dynamic installation via `NETCDF_HOME`.
- Provide helper functions that leverage additional packages such as NumPy and Matplotlib.

---

## Requirements

Before installation, ensure that:
- The ForeFire library is precompiled and the dynamic library (e.g., `libforefireL.dylib`, `libforefireL.so`, or `libforefireL.dll`) is located in the top-level `lib/` directory.
- The ForeFire source (headers) is available in the top-level `src/` directory.
- A compatible NetCDF installation is available. Either:
  - Set `SRC_MESONH` and `XYZ` to enable auto-detection of a static NetCDF installation, or
  - Set `NETCDF_HOME` to the path where NetCDF (and its headers) is installed.
- Python (>=3.8) and a C++17 compiler are installed.
- The following Python packages are required:
  - pybind11
  - numpy
  - matplotlib

---

## Installation

You can build and install the package into your current Python interpreter in editable mode. This mode allows you to recompile the extension without needing to reinstall the package.

1. **Editable Installation**

   In the `bindings/python` directory, run:

   ```bash
   pip install -e .
   ```

   Editable mode links the installed package to the source directory. Any recompilation (e.g., via `python setup.py build_ext --inplace`) will be immediately available.

2. **Wheel Build**

   To build a wheel without installing it directly, run:

   ```bash
   pip wheel .
   ```

   You can later install the generated wheel with:

   ```bash
   pip install <wheel_file>
   ```

---

## Usage

### Verifying the Installation

The first step is to confirm that the PyForeFire library was installed and linked correctly. The following code creates a `ForeFire` instance and defines a simulation domain.

```python
import pyforefire as forefire

# Create an instance of the ForeFire class
ff = forefire.ForeFire()

# Example usage: define a domain command
sizeX = 300
sizeY = 200
myCmd = "FireDomain[sw=(0.,0.,0.);ne=(%f,%f,0.);t=0.]" % (sizeX, sizeY)

# Execute the command
ff.execute(myCmd)
print("PyForeFire installed and domain created successfully.")
```

If this script runs without an `ImportError` or linking error, your installation is working. *Note: You may see warnings about missing fuel tables, which is expected at this stage.*

### Running a Simple Simulation

To see a simulation in action, this example starts a fire in the center of a domain and runs it for 1000 seconds.

```python
import pyforefire as forefire

ff = forefire.ForeFire()

# 1. Define a 10km x 10km simulation domain
sim_shape = (10000, 10000)
ff.execute(f'FireDomain[sw=(0,0,0);ne=({sim_shape[0]},{sim_shape[1]},0);t=0]')

# 2. Set a simple propagation model (isotropic, i.e., a perfect circle)
ff.addLayer("propagation", "Iso", "propagationModel")

# 3. Start a fire in the center of the domain
ff.execute(f'startFire[loc=({sim_shape[0]/2},{sim_shape[1]/2},0.0)]')

# 4. Run the simulation forward by 1000 seconds
ff.execute("step[dt=1000]")

# 5. Print the state of the fire front to the console
print(ff.execute("print[]"))
```

This will produce text output describing the location of the fire front nodes.

To generate a `circle.kml` file for visualization in Google Earth, you can set the `dumpMode` parameter before the final print command:
```python
# Optional: Set output mode to KML and save to a file
ff["dumpMode"] = "kml"
ff.execute("print[circle.kml]")
```

### More Advanced Examples

For more complex examples that use real-world data (like fuel, topography, and wind), please see the scripts located in the `tests/python/` directory of the main repository.

---

## Development

For development or when modifying the underlying C++ code:
- Rebuild the Python extension module in-place using:

  ```bash
  python setup.py build_ext --inplace
  ```

- This approach updates the extension module immediately without the need for a full reinstall.


## Troubleshooting

- **NetCDF Not Found:**  
  If you encounter an error such as `fatal error: 'netcdf' file not found`, ensure that either:
  - Your environment variables `SRC_MESONH` and `XYZ` (for static linking) or `NETCDF_HOME` (for dynamic linking) are correctly set, or
  - Your system includes the necessary NetCDF headers and libraries in the default search paths.

- **Editable Installation Issues:**  
  Ensure you are running the command in the correct directory (`bindings/python`) and that no legacy configuration files (like a `setup.cfg`) conflict with the `pyproject.toml`.

---

## License

This project is licensed under the terms specified in the `LICENSE` file.

---

## Project URLs

- **Homepage:** [https://github.com/forefireAPI/forefire](https://github.com/forefireAPI/forefire)
- **Documentation:** [https://forefire.readthedocs.io/en/latest/](https://forefire.readthedocs.io/en/latest/)