#!/usr/bin/env bash
set -e
IFS=$'\n\t'

echo "====== UNIX REQUIREMENTS ========"

apt-get update

apt install build-essential -y
apt install libnetcdf-c++4-dev -y
apt install cmake -y

echo "==========================="
echo "========= FOREFIRE ========"
echo "==========================="

mkdir -p build
cd build
cmake ../
make

# Determine the absolute path to the bin directory.
BIN_PATH="$(pwd)/bin"

echo "ForeFire has been installed in $BIN_PATH."

# Determine which shell configuration file to use.
# Here we assume bash; you could extend this to support zsh as needed.
CONFIG_FILE="$HOME/.bashrc"

# The line to add:
EXPORT_LINE="export PATH=\"$BIN_PATH:\$PATH\"  # Add ForeFire to PATH"

# Check if the configuration file already contains this export line.
if grep -qF "$BIN_PATH" "$CONFIG_FILE"; then
  echo "ForeFire already appears to be in your PATH in $CONFIG_FILE."
else
  read -p "Do you want to add ForeFire to your PATH permanently in $CONFIG_FILE? (y/n): " answer
  if echo "$answer" | grep -qi '^y$'; then
      echo "export PATH=\"$BIN_PATH:\$PATH\"  # Add ForeFire to PATH" >> "$CONFIG_FILE"
      echo "ForeFire has been added to your PATH in $CONFIG_FILE."
      echo "Please run 'source $CONFIG_FILE' or restart your terminal to update your PATH."
  else
      echo "Please remember to add the following line to your shell configuration file manually:"
      echo "export PATH=\"$BIN_PATH:\$PATH\"  # Add ForeFire to PATH"
  fi
fi