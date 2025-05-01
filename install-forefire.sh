#!/usr/bin/env bash
set -e
IFS=$'\n\t'

# --- parse flags
AUTO_ANS=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    -y|--yes)
      AUTO_ANS=1
      shift
      ;;
    *)
      echo "Usage: $0 [-y|--yes]" >&2
      exit 1
      ;;
  esac
done

echo -e "\n========= FOREFIRE INSTALLER ========"

PROJECT_ROOT="$(pwd)"
echo "Project Root detected as: $PROJECT_ROOT"

echo -e "\n======== UNIX REQUIREMENTS ==========\n"

apt-get update
apt install build-essential -y
apt install libnetcdf-c++4-dev -y
apt install cmake -y

echo -e "\n======= BUILD WITH CMAKE ==========\n"

# Determine the absolute path to the bin directory.
BIN_PATH="$PROJECT_ROOT/bin"

mkdir -p build
cd build
cmake ../
make -j"$(nproc)"

echo -e "\n=== ForeFire has been installed to $BIN_PATH ===\n"

# Determine which shell configuration file to update.
# If the script is run via sudo, use the SUDO_USER's home directory.
if [ -n "${SUDO_USER:-}" ]; then
  CONFIG_FILE="/home/$SUDO_USER/.bashrc"
else
  CONFIG_FILE="$HOME/.bashrc"
fi

PATH_LINE="export PATH=\"$BIN_PATH:\$PATH\""
FOREFIREHOME_LINE="export FOREFIREHOME=\"$PROJECT_ROOT\""

# Check if we've already updated the PATH in this file.
if grep -qF "$BIN_PATH" "$CONFIG_FILE"; then
  echo "ForeFire already appears in your PATH in $CONFIG_FILE."
else
  if (( AUTO_ANS )); then
    ans=y
  else
    read -p "Do you want to add ForeFire to your PATH permanently in $CONFIG_FILE? (y/n): " ans
  fi
  
  if echo "$ans" | grep -qi '^y'; then
    # Append a blank line, a comment, and the export line.
    echo "" >> "$CONFIG_FILE"
    echo "# Add ForeFire to PATH" >> "$CONFIG_FILE"
    echo "$PATH_LINE" >> "$CONFIG_FILE"
    echo "$FOREFIREHOME_LINE" >> "$CONFIG_FILE"
    echo "ForeFire has been added to your PATH in $CONFIG_FILE."
    echo "Please run 'source $CONFIG_FILE' or restart your terminal to update your PATH."
  else
    echo "No changes made. Please add the following line manually to your shell configuration file:"
    echo "$PATH_LINE"
  fi
fi

echo -e "\n=== ForeFire installation completed ===\n"
