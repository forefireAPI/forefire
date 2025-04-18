#!/usr/bin/env bash
set -euo pipefail

FRAMERATE=60

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <file_uint16_NI_NJ.dat> [...]"
  exit 1
fi

for file in "$@"; do
  base=$(basename "$file")
  if [[ $base =~ ^([A-Za-z0-9_]+)_uint16_([0-9]+)_([0-9]+)\.dat$ ]]; then
    var=${BASH_REMATCH[1]}
    W=${BASH_REMATCH[2]}
    H=${BASH_REMATCH[3]}

    webm="../images/${var}.webm"

    echo "▶ Generating ${webm} (threshold α at 32)…"
    ffmpeg -y \
      -f rawvideo -pix_fmt gray16le -s "${W}x${H}" -r "${FRAMERATE}" -crf 15 -i "${file}" \
      -filter_complex "[0:v]transpose=2,format=gray,split[luma][alpha_in];[alpha_in]lut=a='if(gte(val,32),255,0)'[alpha];[luma][alpha]alphamerge,format=yuva420p[out]" \
      -map "[out]" \
      -c:v libvpx-vp9 -auto-alt-ref 0 \
      "${webm}"

    echo "  ✓ done: ${var}"
  else
    echo "✗ Skipping ‘$file’: filename must match VAR_uint16_NI_NJ.dat"
  fi
done
