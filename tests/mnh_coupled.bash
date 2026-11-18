#!/usr/bin/env bash
# From a bare Linux machine to a running ForeFire + Meso-NH coupled case.
# Run with --help for usage. Each step is skipped when already done.

# No `set -u`: Meso-NH's profile reads SRC_MESONH before defining it.
set -eo pipefail

usage() {
  cat <<'EOF'
Usage: tests/mnh_coupled.bash [options]

Builds Meso-NH 6.0.1 and ForeFire (with MPI), links them, and runs the
tests/mnh_real_nested case (Pedrógão Grande, 4 MPI processes, about 7 min).
Steps that are already done are skipped, so running it again is cheap.

Options:
  --workdir DIR     where Meso-NH, the ForeFire builds and the runs go
                    (default: $FF_MNH_WORKDIR or ~/forefire-mnh)
  --tarball FILE    use this Meso-NH 6.0.1 tarball instead of downloading it
                    (default: $MNH_TARBALL)
  --jobs N          parallel build jobs (default: all cores)
  --compare REF     instead of a single run, build a second ForeFire from REF
                    (a branch, tag or commit of this repository, or a directory
                    with a ForeFire source tree), run this tree, REF, this tree,
                    REF, and compare the outputs byte for byte. Exits 1 if a
                    file differs between the two builds but not between the
                    two runs of this tree.
  -h, --help        show this help

Needs Bash, about 10 GB of disk and port 8080 free (the case starts
ForeFire's HTTP server there). Never runs sudo: missing packages are listed
with the apt command to install them.
EOF
}

# --- settings -----------------------------------------------------------------

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CASE="tests/mnh_real_nested"
PORT=8080

MNH_NAME="mesonh-code-PACK-MNH-V6-0-1"
MNH_URL="https://zenodo.org/records/21395048/files/$MNH_NAME.tar.gz?download=1"
MNH_MD5="ed745f20435d5c0afbfc269b29513da5"   # published by Zenodo

WORKDIR="${FF_MNH_WORKDIR:-$HOME/forefire-mnh}"
TARBALL="${MNH_TARBALL:-}"
JOBS="$(nproc 2>/dev/null || echo 4)"
COMPARE=""

while [ $# -gt 0 ]; do
  case "$1" in
    --workdir) WORKDIR="$2"; shift 2 ;;
    --tarball) TARBALL="$2"; shift 2 ;;
    --jobs)    JOBS="$2"; shift 2 ;;
    --compare) COMPARE="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1 (see --help)" >&2; exit 2 ;;
  esac
done

mkdir -p "$WORKDIR/logs"
WORKDIR="$(cd "$WORKDIR" && pwd)"
LOGS="$WORKDIR/logs"
MNH_DIR="$WORKDIR/$MNH_NAME"

# --- helpers ------------------------------------------------------------------

die() { printf '\nerror: %s\n' "$*" >&2; exit 1; }

TIMINGS=()
STEP_NAME="" STEP_START=0
step() {
  step_done
  STEP_NAME="$1" STEP_START=$SECONDS
  printf '\n==> %s\n' "$1"
}
step_done() {
  [ -n "$STEP_NAME" ] || return 0
  local s=$((SECONDS - STEP_START))
  TIMINGS+=("$(printf '%4dm%02ds  %s' $((s / 60)) $((s % 60)) "$STEP_NAME")")
  STEP_NAME=""
}
print_timings() {
  step_done
  printf '\nTime per step:\n'
  printf '  %s\n' "${TIMINGS[@]}"
}

# Run a command with its output in a log file; show the end of it on failure.
logged() {
  local log="$1"; shift
  echo "    log: $log"
  if ! "$@" >"$log" 2>&1; then
    tail -n 30 "$log" >&2
    die "failed: $* (full log: $log)"
  fi
}

md5_ok() { [ "$(md5sum "$1" | cut -d' ' -f1)" = "$MNH_MD5" ]; }

port_busy() { (exec 3<>"/dev/tcp/127.0.0.1/$PORT") 2>/dev/null; }

# --- 1. prerequisites ---------------------------------------------------------

check_prerequisites() {
  step "1. Prerequisites"
  local missing=() pair
  for pair in gcc:build-essential g++:build-essential make:build-essential \
              gfortran:gfortran mpicc:libopenmpi-dev mpif90:libopenmpi-dev \
              mpiexec:openmpi-bin cmake:cmake git:git git-lfs:git-lfs \
              curl:curl nc-config:libnetcdf-dev ncxx4-config:libnetcdf-c++4-dev; do
    command -v "${pair%%:*}" >/dev/null || missing+=("${pair#*:}")
  done
  if [ ${#missing[@]} -gt 0 ]; then
    echo "Missing packages. Install them with:"
    echo "  sudo apt install $(printf '%s\n' "${missing[@]}" | sort -u | tr '\n' ' ')"
    die "prerequisites missing"
  fi
  echo "    all found"
}

# --- 2. Meso-NH sources -------------------------------------------------------

get_mesonh() {
  step "2. Meso-NH 6.0.1 tarball"
  if [ -n "$TARBALL" ]; then
    [ -f "$TARBALL" ] || die "tarball not found: $TARBALL"
    md5_ok "$TARBALL" || die "$TARBALL is not the Zenodo 6.0.1 tarball (MD5 mismatch)"
    echo "    using $TARBALL (checksum OK)"
    return
  fi
  TARBALL="$WORKDIR/$MNH_NAME.tar.gz"
  if [ -f "$TARBALL" ] && md5_ok "$TARBALL"; then
    echo "    already downloaded: $TARBALL"
    return
  fi
  echo "    downloading from Zenodo (190 MB)"
  curl -fL --retry 3 -o "$TARBALL.part" "$MNH_URL"
  md5_ok "$TARBALL.part" || die "downloaded file has the wrong MD5; try again"
  mv "$TARBALL.part" "$TARBALL"
}

# --- 3. Meso-NH build ---------------------------------------------------------

load_mesonh_profile() {
  local profile
  profile="$(ls "$MNH_DIR"/conf/profile_mesonh-LXgfortran-*-MPIAUTO-O2 2>/dev/null | head -n 1)"
  [ -n "$profile" ] || die "no Meso-NH profile in $MNH_DIR/conf"
  # The profile warns when SRC_MESONH is already set, and does not expect -e.
  unset SRC_MESONH XYZ
  set +e
  . "$profile" >/dev/null
  set -e
  [ -n "$SRC_MESONH" ] && [ -n "$XYZ" ] || die "loading $profile did not set SRC_MESONH and XYZ"
}

build_mesonh() {
  step "3. Meso-NH build"
  if [ ! -f "$MNH_DIR/.extracted" ]; then
    echo "    extracting into $WORKDIR"
    rm -rf "$MNH_DIR"
    tar xzf "$TARBALL" -C "$WORKDIR"
    touch "$MNH_DIR/.extracted"
  fi
  if ! ls "$MNH_DIR"/conf/profile_mesonh-LXgfortran-*-MPIAUTO-O2 >/dev/null 2>&1; then
    echo "    configure"
    (cd "$MNH_DIR/src" && export ARCH=LXgfortran VER_MPI=MPIAUTO OPTLEVEL=O2 &&
       logged "$LOGS/mesonh-configure.log" ./configure)
  fi
  load_mesonh_profile
  if [ -x "$SRC_MESONH/exe/MESONH$XYZ" ]; then
    echo "    already built: $SRC_MESONH/exe/MESONH$XYZ"
    return
  fi
  echo "    make -j $JOBS (about 15 min on 12 cores)"
  (cd "$SRC_MESONH/src" && logged "$LOGS/mesonh-make.log" make -j "$JOBS" &&
     logged "$LOGS/mesonh-install.log" make installmaster)
}

# --- 4. ForeFire build --------------------------------------------------------

# build_forefire SRC NAME: builds the ForeFire library of source tree SRC and
# copies it to $WORKDIR/forefire/NAME/libforefireL.so.
build_forefire() {
  local src="$1" dest="$WORKDIR/forefire/$2"
  echo "    $src -> $dest"
  mkdir -p "$dest"
  logged "$dest/cmake.log" cmake -S "$src" -B "$dest/build" \
    -DFOREFIRE_ENABLE_MPI=ON -DFOREFIRE_CHECK_LFS=OFF
  grep -q 'MPI \.* ON' "$dest/cmake.log" || die "CMake did not find MPI (see $dest/cmake.log)"
  # CMake writes the library into SRC/lib whatever the build directory.
  # Removing it forces a relink from this build directory, and the copy keeps
  # it safe from the next build of the same tree.
  rm -f "$src/lib/libforefireL.so"
  logged "$dest/build.log" cmake --build "$dest/build" --target forefireL -j "$JOBS"
  cp "$src/lib/libforefireL.so" "$dest/libforefireL.so"
}

# --- 5. link ------------------------------------------------------------------

link_forefire() {
  ln -sfn "$1" "$SRC_MESONH/exe/libForeFire.so"
  echo "    $SRC_MESONH/exe/libForeFire.so -> $1"
}

# --- 6. run -------------------------------------------------------------------

# The case's NetCDF inputs are in Git LFS; a clone without it has text stubs.
is_lfs_stub() { head -c 200 "$1" | grep -q 'git-lfs.github.com/spec'; }

check_case_data() {
  local probe="$REPO/$CASE/RUN12.1.PRUN1.004.nc"
  if is_lfs_stub "$probe"; then
    echo "    fetching the case's Git LFS files"
    git -C "$REPO" lfs pull --include "$CASE/**"
    ! is_lfs_stub "$probe" || die "$probe is still a Git LFS stub"
  fi
  echo "    case data present"
}

# run_case DIR FOREFIREHOME: fresh copy of the case in DIR, then run.bash.
run_case() {
  local dir="$1" home="$2"
  rm -rf "$dir"
  mkdir -p "$(dirname "$dir")"
  cp -r "$REPO/$CASE" "$dir"
  (cd "$dir" && bash clean.bash)
  port_busy && die "port $PORT is in use; stop whatever listens there and run again"
  echo "    running in $dir (about 7 min), log: $dir/run.log"
  if ! (cd "$dir" && FOREFIREHOME="$home" bash run.bash) >"$dir/run.log" 2>&1; then
    tail -n 30 "$dir/run.log" >&2
    die "run failed (full log: $dir/run.log)"
  fi
  echo "    ForeFire.0.nc: $(wc -c <"$dir/ForeFire/Outputs/ForeFire.0.nc") bytes"
}

# --- 7. comparison ------------------------------------------------------------

# Files that differ between two runs of the same build.
known_noise() {
  case "$1" in
    RUN12.*.FIRE.*.nc|MODEL*/output.*.air_density) return 0 ;;
    *) return 1 ;;
  esac
}

# Source tree for --compare: a directory as given, or REF exported from git.
compare_source() {
  local ref="$1" commit dest="$WORKDIR/src/compare"
  if [ -d "$ref" ]; then
    (cd "$ref" && pwd)
    return
  fi
  commit="$(git -C "$REPO" rev-parse --verify --quiet "$ref^{commit}" ||
            git -C "$REPO" rev-parse --verify --quiet "origin/$ref^{commit}")" ||
    die "--compare $ref: not a directory, nor a branch, tag or commit of $REPO"
  if [ "$(cat "$dest/.commit" 2>/dev/null)" != "$commit" ]; then
    rm -rf "$dest"
    mkdir -p "$dest"
    git -C "$REPO" archive "$commit" | tar x -C "$dest"
    echo "$commit" >"$dest/.commit"
  fi
  echo "$dest"
}

same() { cmp -s "$1" "$2"; }

compare_runs() {
  local a1="$1/this-1" b1="$1/other-1" a2="$1/this-2" b2="$1/other-2"
  local f files real=0 noise=0 unexpected=0 identical=0
  files="$(for d in "$a1" "$b1"; do
             (cd "$d" && find ForeFire/Outputs MODEL1 MODEL2 -type f && ls RUN12.*.FIRE.*.nc)
           done 2>/dev/null | sort -u)" || true
  printf '\n'
  while IFS= read -r f; do
    [ -n "$f" ] || continue
    if [ ! -f "$a1/$f" ] || [ ! -f "$b1/$f" ]; then
      echo "    REAL    $f  (only in one of the two builds' outputs)"
      real=$((real + 1))
    elif same "$a1/$f" "$b1/$f" && same "$a2/$f" "$b2/$f"; then
      identical=$((identical + 1))
    elif ! same "$a1/$f" "$a2/$f"; then
      if known_noise "$f"; then
        noise=$((noise + 1))
      else
        echo "    NOISE?  $f  (differs between the two runs of this tree too; not in the known list)"
        unexpected=$((unexpected + 1))
      fi
    else
      echo "    REAL    $f"
      real=$((real + 1))
    fi
  done <<< "$files"
  printf '\n    %d identical, %d known noise, %d unexpected noise, %d real differences\n' \
    "$identical" "$noise" "$unexpected" "$real"
  [ "$real" -eq 0 ]
}

# --- main ---------------------------------------------------------------------

echo "ForeFire: $REPO"
echo "Work dir: $WORKDIR"
trap 'print_timings' EXIT

check_prerequisites
get_mesonh
build_mesonh

step "4. ForeFire build (MPI)"
build_forefire "$REPO" this
if [ -n "$COMPARE" ]; then
  OTHER_SRC="$(compare_source "$COMPARE")"
  build_forefire "$OTHER_SRC" other
fi

step "5. Link and case data"
link_forefire "$WORKDIR/forefire/this/libforefireL.so"
check_case_data

RUN_ID="$(date +%Y%m%d-%H%M%S)"
if [ -z "$COMPARE" ]; then
  step "6. Run $CASE"
  echo "    To watch it in 3D, open http://localhost:$PORT/mnh3d.html while it runs"
  run_case "$WORKDIR/runs/$RUN_ID" "$REPO"
  echo "    done: $WORKDIR/runs/$RUN_ID"
  exit 0
fi

step "7. Compare this tree with $COMPARE"
echo "    Four runs of about 7 min each. Keep the 3D page closed: it sends commands to the run."
ROOT="$WORKDIR/runs/compare-$RUN_ID"
for run in this-1 other-1 this-2 other-2; do
  if [ "${run%-*}" = this ]; then src="$REPO"; else src="$OTHER_SRC"; fi
  link_forefire "$WORKDIR/forefire/${run%-*}/libforefireL.so"
  run_case "$ROOT/$run" "$src"
done
# Leave Meso-NH linked to this tree, as after a plain run.
link_forefire "$WORKDIR/forefire/this/libforefireL.so"
compare_runs "$ROOT"
