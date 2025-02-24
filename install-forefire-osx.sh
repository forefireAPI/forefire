#!/bin/bash
echo "====== MAC OS REQUIREMENTS ========"

# Update Homebrew.
brew update

# Ensure Xcode Command Line Tools are installed.
if ! xcode-select -p > /dev/null 2>&1; then
    echo "Xcode Command Line Tools not found. Installing..."
    xcode-select --install
fi

# Install dependencies via Homebrew.
brew install cmake
brew install netcdf
brew install netcdf-cxx

# Set NETCDF_HOME to the prefix for the NetCDF C library.
export NETCDF_HOME=$(brew --prefix netcdf)
echo "NETCDF_HOME set to $NETCDF_HOME"

# Set NETCDF_CXX_HOME to the prefix for the NetCDF C++ library.
export NETCDF_CXX_HOME=$(brew --prefix netcdf-cxx)
echo "NETCDF_CXX_HOME set to $NETCDF_CXX_HOME"

# Create a symlink in NETCDF_HOME/include so that "netcdf" points to "netcdf.h"
cd "$NETCDF_HOME/include"
if [ ! -e netcdf ]; then
    echo "Creating symlink for netcdf -> netcdf.h"
    ln -s netcdf.h netcdf
fi
cd - > /dev/null

# Now, create a symlink in the netcdf-cxx lib directory.
cd "$NETCDF_CXX_HOME/lib"
# Expect the Homebrew formula to install the library as "libnetcdf-cxx4.dylib"
if [ -e libnetcdf-cxx4.dylib ]; then
    if [ ! -e libnetcdf_c++4.dylib ]; then
        echo "Creating symlink: libnetcdf_c++4.dylib -> libnetcdf-cxx4.dylib"
        ln -s libnetcdf-cxx4.dylib libnetcdf_c++4.dylib
    else
        echo "Symlink for netcdf_c++4 already exists."
    fi
else
    echo "Error: Expected libnetcdf-cxx4.dylib not found in $NETCDF_CXX_HOME/lib"
fi
cd - > /dev/null

echo "==========================="
echo "========= FOREFIRE ========"
echo "==========================="

# Create build directory, configure, and compile ForeFire.
mkdir -p build
cd build

# Pass include flags for both netcdf-cxx and netcdf.
cmake -D NETCDF_HOME=$NETCDF_HOME \
      -DCMAKE_CXX_FLAGS="-I$NETCDF_CXX_HOME/include -I$NETCDF_HOME/include" \
      ../
make
