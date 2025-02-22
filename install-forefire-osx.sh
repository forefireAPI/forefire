#!/bin/bash
echo "====== MAC OS REQUIREMENTS ========"

# Update Homebrew
brew update

# Optionally ensure that Xcode Command Line Tools are installed:
if ! xcode-select -p > /dev/null 2>&1; then
    echo "Xcode Command Line Tools not found. Installing..."
    xcode-select --install
fi

# Install required packages via Homebrew.
brew install cmake
brew install netcdf-cxx-legacy   # Equivalent to libnetcdf-c++4-dev on Linux

echo "==========================="
echo "========= FOREFIRE ========"
echo "==========================="

# Create build directory, configure, and compile ForeFire.
mkdir -p build
cd build
cmake ../
make
