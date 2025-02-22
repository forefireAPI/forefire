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

# Create a symbolic link in the include directory so that "netcdf" points to "netcdf.h"
cd "$NETCDF_HOME/include"
if [ ! -e netcdf ]; then
    echo "Creating symlink for netcdf -> netcdf.h"
    ln -s netcdf.h netcdf
fi
cd - > /dev/null

echo "==========================="
echo "========= FOREFIRE ========"
echo "==========================="

# Create build directory, configure, and compile ForeFire.
mkdir -p build
cd build

# Pass the include flag explicitly.
cmake -D NETCDF_HOME=$NETCDF_HOME \
      -DCMAKE_CXX_FLAGS="-I$NETCDF_HOME/include" \
      ../
make
