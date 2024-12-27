import os
import sys
import platform
from setuptools import setup, find_packages
from pybind11.setup_helpers import Pybind11Extension, build_ext

# Retrieve environment variables
NETCDF_DIR = os.environ.get('NETCDF_DIR')
NETCDF_CXX_DIR = os.environ.get('NETCDF_CXX_DIR')
FOREFIRE_DIR = os.environ.get('FOREFIRE_DIR')
FOREFIRE_LIB = os.environ.get('FOREFIRE_LIB', 'forefireL')

# Ensure required environment variables are set
if not NETCDF_DIR or not NETCDF_CXX_DIR or not FOREFIRE_DIR:
    print('NETCDF_DIR, NETCDF_CXX_DIR, and FOREFIRE_DIR environment variables must all be set.')
    sys.exit(1)

# Determine the platform-specific library extension
current_platform = platform.system()
if current_platform == 'Darwin':  # macOS
    lib_ext = 'dylib'
elif current_platform == 'Linux':
    lib_ext = 'so'
elif current_platform == 'Windows':
    lib_ext = 'dll'
else:
    raise RuntimeError(f'Unsupported platform: {current_platform}')

libraries = [FOREFIRE_LIB]
library_dirs = [os.path.join(FOREFIRE_DIR, 'lib')]

# Define extra objects based on the platform
extra_objects = [
    os.path.join(FOREFIRE_DIR, 'lib', f'lib{FOREFIRE_LIB}.{lib_ext}')
]

# Define the extension module
ext_modules = [
    Pybind11Extension(
        "pyforefire._pyforefire",  # Namespaced within the package with a leading underscore
        ["src/pyforefire/_pyforefire.cpp"],  # Updated file name
        include_dirs=[
            os.path.join(FOREFIRE_DIR, 'src'),
            os.path.join(FOREFIRE_DIR, 'src', 'include'),
            os.path.join(NETCDF_DIR, 'include'),
            os.path.join(NETCDF_CXX_DIR, 'include')
        ],
        libraries=libraries,
        library_dirs=library_dirs,
        runtime_library_dirs=library_dirs if current_platform != 'Windows' else [],
        extra_objects=extra_objects,
        language='c++'
    ),
]

# Read the long description from README.md
with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

# Setup configuration
setup(
    name="pyforefire",
    version="2024.1",
    install_requires=[
        "pybind11",
        "setuptools",
        "wheel",
        # Add other dependencies here if needed
    ],
    author="Jean-Baptiste Filippi",
    author_email="filippi_j@univ-corse.fr",
    description="Python version of ForeFire library",
    long_description=long_description,
    long_description_content_type="text/markdown",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    packages=find_packages(where='src'),  # Automatically find packages under src/
    package_dir={'': 'src'},  # Tell setuptools to look for packages in src/
    package_data={
        'pyforefire': [f'lib{FOREFIRE_LIB}.{lib_ext}']
    },
    include_package_data=True,
    zip_safe=False,
    classifiers=[
        "Programming Language :: Python :: 3",
        "Programming Language :: C++",
        "Operating System :: OS Independent",
    ],
    python_requires='>=3.8',
    # Metadata from pyproject.toml can be omitted here if already specified
)
