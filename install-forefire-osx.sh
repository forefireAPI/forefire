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
