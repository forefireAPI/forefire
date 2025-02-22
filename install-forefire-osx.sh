#!/bin/bash
echo "====== MAC OS REQUIREMENTS ========"

# Update Homebrew
brew update

# Ensure Xcode Command Line Tools are installed.
if ! xcode-select -p > /dev/null 2>&1; then
    echo "Xcode Command Line Tools not found. Installing..."
    xcode-select --install
fi

# Install dependencies via Homebrew.
brew install cmake
brew install netcdf-cxx

# Set NETCDF_HOME to the Homebrew prefix for netcdf-cxx.
export NETCDF_HOME=$(brew --prefix netcdf-cxx)
echo "NETCDF_HOME set to $NETCDF_HOME"

echo "==========================="
echo "========= FOREFIRE ========"
echo "==========================="

# Create build directory, configure, and compile ForeFire.
mkdir -p build
cd build
cmake -D NETCDF_HOME=$NETCDF_HOME ../
make
