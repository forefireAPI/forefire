#!/usr/bin/env bash
set -e
IFS=$'\n\t'

echo -e "\n========= FOREFIRE INSTALLER ========"

echo -e "\n======== UNIX REQUIREMENTS ==========\n"

apt-get update

apt install build-essential -y
apt install libnetcdf-c++4-dev -y
apt install cmake -y

echo -e "\n======= BUILD WITH CMAKE ==========\n"

mkdir -p build
cd build
cmake ../
make

# The bin directory where the compiled ForeFire executable is placed
BIN_PATH="$(pwd)/bin"
echo -e "\n=== ForeFire has been installed to $BIN_PATH ===\n"

# The shell config file to update (assuming bash)
CONFIG_FILE="$HOME/.bashrc"

# The line we'll add if the user wants to update their PATH
PATH_LINE="export PATH=\"$BIN_PATH:\$PATH\"  # Add ForeFire to PATH"

# Check if we've already updated the PATH in this file
if grep -qF "$BIN_PATH" "$CONFIG_FILE"; then
  echo "ForeFire already appears in your PATH in $CONFIG_FILE."
else
  # Prompt the user for permission
  read -p "Add ForeFire to your PATH permanently in $CONFIG_FILE? (y/n): " ans
  # POSIX-compatible check for 'y' or 'Y'
  if echo "$ans" | grep -qi '^y'; then
    # Optionally add a small comment first
    echo -e "\n# ForeFire PATH" >> "$CONFIG_FILE"
    echo "$PATH_LINE" >> "$CONFIG_FILE"
    echo "ForeFire has been added to your PATH in $CONFIG_FILE."
    echo "Please run 'source $CONFIG_FILE' or restart your terminal to update your PATH."
  else
    echo "No changes made. Please add this line manually to your shell config file if desired:"
    echo "$PATH_LINE"
  fi
fi

echo -e "\n=== ForeFire instalation completed ===\n"
