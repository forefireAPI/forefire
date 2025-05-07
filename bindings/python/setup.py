import os
import platform
import glob
from setuptools import setup, find_packages
from pybind11.setup_helpers import Pybind11Extension, build_ext

# Determine current directory (where setup.py resides)
this_dir = os.path.abspath(os.path.dirname(__file__))
# Compute the top-level ForeFire directory (two levels up: bindings/python -> bindings -> forefire)
FOREFIRE_DIR = os.path.abspath(os.path.join(this_dir, "..", ".."))
FOREFIRE_LIB = "forefireL"  # as used in lib/libforefireL.dylib (or .so/.dll)

# Determine platform-specific library extension.
current_platform = platform.system()
if current_platform == "Darwin":
    lib_ext = "dylib"
elif current_platform == "Linux":
    lib_ext = "so"
elif current_platform == "Windows":
    lib_ext = "dll"
else:
    raise RuntimeError("Unsupported platform: " + current_platform)

# ForeFire precompiled library directory.
forefire_lib_dir = os.path.join(FOREFIRE_DIR, "lib")

# --- Determine NetCDF configuration ---
# Initialize lists for additional include and library directories, and libraries to link.
netcdf_include_dirs = []
netcdf_library_dirs = []
netcdf_libraries = []

# Option 1: Use MESONH and XYZ to auto-detect a static NetCDF installation.
if os.environ.get("SRC_MESONH") and os.environ.get("XYZ"):
    mesonh = os.environ["SRC_MESONH"]
    xyz = os.environ["XYZ"]
    pattern = os.path.join(mesonh, "src", "dir_obj" + xyz, "MASTER", "NETCDF-*")
    netcdf_dirs = glob.glob(pattern)
    if netcdf_dirs:
        NETCDF_HOME = netcdf_dirs[0]
        print("Detected NETCDF_HOME:", NETCDF_HOME)
        # (In your CMake you use the lib64 directory for static linking.)
        netcdf_include_dirs.append(os.path.join(NETCDF_HOME, "include"))
        netcdf_library_dirs.append(os.path.join(NETCDF_HOME, "lib64"))
        # For static linking you might want to add extra_objects (see below).
        # Here we assume dynamic linking for simplicity:
        netcdf_libraries.extend(["netcdf", "netcdf_c++4"])
    else:
        raise RuntimeError(f"No NETCDF-* directory found under {mesonh}/src/dir_obj{xyz}/MASTER/")
# Option 2: Use NETCDF_HOME if defined.
elif os.environ.get("NETCDF_HOME"):
    NETCDF_HOME = os.environ["NETCDF_HOME"]
    print("NETCDF_HOME set to:", NETCDF_HOME)
    netcdf_include_dirs.append(os.path.join(NETCDF_HOME, "include"))
    netcdf_library_dirs.append(os.path.join(NETCDF_HOME, "lib"))
    netcdf_libraries.extend(["netcdf", "netcdf_c++4"])
else:
    print("Warning: Neither (SRC_MESONH and XYZ) nor NETCDF_HOME are set. "
          "Attempting to use system default include and library paths for NetCDF.")

# --- Define the Pybind11 extension ---
ext_modules = [
    Pybind11Extension(
        "pyforefire._pyforefire",
        ["src/pyforefire/_pyforefire.cpp"],
        include_dirs=[
            os.path.join(FOREFIRE_DIR, "src"),
            os.path.join(FOREFIRE_DIR, "src", "include")
        ] + netcdf_include_dirs,
        libraries=[FOREFIRE_LIB] + netcdf_libraries,
        library_dirs=[forefire_lib_dir] + netcdf_library_dirs,
        # On non-Windows platforms, specify runtime library directories so the dynamic linker finds the libs.
        runtime_library_dirs=([forefire_lib_dir] + netcdf_library_dirs) if current_platform != "Windows" else [],
        # Link the precompiled ForeFire library explicitly.
        extra_objects=[os.path.join(forefire_lib_dir, f"lib{FOREFIRE_LIB}.{lib_ext}")],
        extra_compile_args=["-std=c++17"],
        language="c++"
    )
]

with open(os.path.join(this_dir, "README.md"), "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup(
    name="pyforefire",
    version="2025.1",
    install_requires=["pybind11", "setuptools", "wheel"],
    author="Jean-Baptiste Filippi",
    author_email="filippi_j@univ-corse.fr",
    description="Python bindings for ForeFire library",
    long_description=long_description,
    long_description_content_type="text/markdown",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    packages=find_packages(where="src"),
    package_dir={"": "src"},
    include_package_data=True,
    zip_safe=False,
    classifiers=[
        "Programming Language :: Python :: 3",
        "Programming Language :: C++",
        "Operating System :: OS Independent"
    ],
    python_requires=">=3.8",
)
