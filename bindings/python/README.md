# PyForeFire

PyForeFire provides Python bindings for the ForeFire library, enabling users to access ForeFire’s functionality directly from Python. The bindings link against the precompiled ForeFire library and include support for NetCDF and other dependencies.

---

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

After installation, you can use PyForeFire in your Python code as follows:

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
```

Additionally, helper modules are provided which make use of NumPy and Matplotlib for data processing and visualization.

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
- **Repository:** [https://github.com/forefireAPI/forefire](https://github.com/forefireAPI/forefire)
