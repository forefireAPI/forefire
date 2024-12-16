#!/bin/bash
echo "====== UNIX REQUIREMENTS ========"

apt-get update

apt install build-essential -y

apt install libnetcdf-c++4-dev -y
apt install cmake -y

echo "==========================="
echo "========= FOREFIRE ========"
echo "==========================="

mkdir build
cd build
cmake ../
make