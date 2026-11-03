#!/usr/bin/env bash
# Run under bash even when called as "sh install-forefire.sh".
if [ -z "${BASH_VERSION:-}" ]; then exec bash "$0" "$@"; fi
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

# --- colours only on a terminal (CI logs stay plain); NO_COLOR turns them off
if [ -t 1 ] && [ -z "${NO_COLOR:-}" ] && [ "${TERM:-dumb}" != "dumb" ]; then
  c() { printf '\033[38;5;%sm' "$1"; }
  BOLD=$'\033[1m'; DIM=$'\033[2m'; RESET=$'\033[0m'
else
  c() { :; }
  BOLD=""; DIM=""; RESET=""
fi

# Section header: step number, title, and a rule underneath.
step() {
  printf '\n\n%s%s[%s/3]%s %s%s%s\n' "$(c 208)" "$BOLD" "$1" "$RESET" "$BOLD" "$2" "$RESET"
  printf '%s──────────────────────────────────────────%s\n\n' "$(c 208)$DIM" "$RESET"
}

PROJECT_ROOT="$(pwd)"
BIN_PATH="$PROJECT_ROOT/bin"
VERSION="$(sed -n 's/.*ff_version *= *"\(v[^"]*\)".*/\1/p' src/include/Version.h 2>/dev/null)"

# --- logo: the FF of the interactive shell, burning from yellow to red
printf '\n'
printf '%s               .    *     .%s\n' "$(c 214)$DIM" "$RESET"
printf '%s            *    .    *      .%s\n' "$(c 208)$DIM" "$RESET"
printf '%s            ██████╗ ██████╗%s\n' "$(c 226)" "$RESET"
printf '%s            ██╔═══╝ ██╔═══╝%s\n' "$(c 220)" "$RESET"
printf '%s            ██████╗ ██████╗%s\n' "$(c 214)" "$RESET"
printf '%s            ██╔═══╝ ██╔═══╝%s\n' "$(c 208)" "$RESET"
printf '%s            ██║     ██║%s\n'     "$(c 202)" "$RESET"
printf '%s            ╚═╝     ╚═╝%s\n'     "$(c 196)" "$RESET"
printf '\n'
printf '            %sForeFire installer%s %s%s%s\n' "$BOLD" "$RESET" "$DIM" "${VERSION}" "$RESET"
printf '            %s%s%s\n' "$DIM" "$PROJECT_ROOT" "$RESET"

step 1 "Linux requirements"

echo "apt-get: build-essential, libnetcdf-c++4-dev, cmake"
apt-get update -qq
apt-get install -y -qq build-essential libnetcdf-c++4-dev cmake
echo "$(c 34)✔$RESET requirements installed"

step 2 "Building ForeFire"

mkdir -p build
cd build
cmake ../
make -j"$(nproc)"

echo
echo "$(c 34)✔$RESET ForeFire built in $BIN_PATH"

step 3 "PATH"

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
  echo "$(c 34)✔$RESET ForeFire already in your PATH ($CONFIG_FILE)"
else
  if (( AUTO_ANS )); then
    ans=y
  else
    read -p "Add ForeFire to your PATH permanently in $CONFIG_FILE? (y/n): " ans
  fi

  if echo "$ans" | grep -qi '^y'; then
    # Append a blank line, a comment, and the export line.
    echo "" >> "$CONFIG_FILE"
    echo "# Add ForeFire to PATH" >> "$CONFIG_FILE"
    echo "$PATH_LINE" >> "$CONFIG_FILE"
    echo "$FOREFIREHOME_LINE" >> "$CONFIG_FILE"
    echo "$(c 34)✔$RESET ForeFire added to your PATH in $CONFIG_FILE"
    echo "  Run 'source $CONFIG_FILE' or open a new terminal."
  else
    echo "No changes made. Add this line to your shell configuration file:"
    echo "  $PATH_LINE"
  fi
fi

printf '\n\n%s%s🔥 ForeFire is ready.%s Try: %sforefire -v%s\n\n' "$(c 208)" "$BOLD" "$RESET" "$BOLD" "$RESET"
